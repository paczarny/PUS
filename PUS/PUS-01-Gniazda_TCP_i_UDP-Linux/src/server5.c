/*Na podstawie:
 * Data:                2009-02-10
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc server1.c -o server1
 * Uruchamianie:        $ ./server1 <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h> // select, poll
#include <sys/types.h>
#include <netinet/in.h> // htons, htonl, ntohs, ntohl
#include <arpa/inet.h>  // inet_pton, inet_ntop
#include <dirent.h>
#include <pthread.h>

char *getFilename(char *fileName, char *fullPath);
//obsluga watku
void *thread_function(void *);
//stworzenie html
void create_html(char *html);
//strona
char html[5120];

int main(int argc, char **argv)
{

    /* Deskryptory dla serwera i klienta: */
    int server_socket, client_socket;

    int retval; /* Wartosc zwracana przez funkcje. */

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t client_addr_len, server_addr_len;

    //jesli nie podano portu to zakoncz dzialanie
    if (argc != 2)
    {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla serwera */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej serwera: */
    memset(&server_addr, 0, sizeof(server_addr));
    /* Domena komunikacyjna (rodzina protokolow): */
    server_addr.sin_family = AF_INET;
    /* Adres nieokreslony (ang. wildcard address): */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* Numer portu: */
    server_addr.sin_port = htons(atoi(argv[1]));
    /* Rozmiar struktury adresowej serwera w bajtach: */
    server_addr_len = sizeof(server_addr);

    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(server_socket, (struct sockaddr *)&server_addr, server_addr_len) == -1)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    //stworzenie strony
    create_html(html);

    /* Przeksztalcenie gniazda w gniazdo nasluchujace: */
    if (listen(server_socket, 100) == -1)
    {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    while (1)
    {

        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1)
        {
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        fprintf(stdout, "Po accepcie %d..\n", client_socket);

        // Tworzenie wątku dla gniazda połączonego:
        pthread_t tids[FD_SETSIZE];
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        retval = pthread_create(&tids[client_socket], &attr, thread_function, &client_socket);
        if (retval != 0)
        {
            perror("pthread_create()");
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}

void create_html(char *html)
{
    const char *cont_b = "<html>\n<head>\n\t<title>PUS</title>\n</head>\n<body>\n<h1>Projektowanie uslug sieciowych.</h1>\n<div style=\"width: 90%; margin: 0 auto; border: dashed 1px #000; text-align: center;\">";
    const char *cont_e = "\n</div>\n</body>\n</html>";
    char tmp[5024];

    struct dirent *de;
    DIR *dir = opendir("img");
    if (dir == NULL)
    {
        perror("opendir()");
        exit(EXIT_FAILURE);
    }
    strcat(html, cont_b);
    while ((de = readdir(dir)) != NULL)
    {
        if (strcmp(".", de->d_name) == 0 || strcmp("..", de->d_name) == 0)
            continue;
        sprintf(tmp, "\n\t<img src=\"img/%s\" alt=\"img\" width=\"400\"/><br>", de->d_name);
        strcat(html, tmp);
    }
    strcat(html, cont_e);
}

void *thread_function(void *argv)
{

    int *client = (int *)argv;
    int client_socket = *client;

    fprintf(stdout, "Jestem w watku %d\n", client_socket);

    char buffer[1024];
    int ret;
    const char *headers, *content;
    char b[1024];

    // Odebranie rządania HTTP:
    ret = recv(client_socket, buffer, sizeof(buffer), 0);
    if (ret < 0)
    {
        perror("recv()");
        close(client_socket);
        return NULL;
    }
    buffer[ret] = '\0';

    // Wysłanie odpowiedzi:
    if (strstr(buffer, ".jpg") || strstr(buffer, ".jpeg") || strstr(buffer, ".png") || strstr(buffer, ".gif"))
    {

        memset((void *)b, 0, sizeof(b));
        char filename[1024];
        getFilename(filename, buffer);

        const char *mime = NULL;
        fpos_t filesize;

        headers = "HTTP/1.1 200 OK\n"
                  "Content-Type: %s\n"
                  "Accept-Ranges: bytes\n"
                  "Content-Disposition: inline filename=\"%s\"\n"
                  "Accept-Ranges: binary\n"
                  "Pragma: public\n"
                  "Expires: 0\n"
                  "Cache-Control: must-revalidate, post-check=0, pre-check=0\n"
                  "Cache-Control: no-cache\n"
                  "Content-Transfer-Encoding: binary\n"
                  "Connection: close\n"
                  "Content-Length: %d\n\r\n";

        // Rodzaj pliku:
        if (strstr(buffer, ".jpg") || strstr(buffer, ".jpeg"))
            mime = "image/jpeg";
        else if (strstr(buffer, ".png"))
            mime = "image/png";
        else if (strstr(buffer, ".png"))
            mime = "image/gif";

        // Rozmiar pliku:
        FILE *fd = fopen(filename, "rb");
        if (fd)
        {
            fseek(fd, 0, SEEK_END);
            fgetpos(fd, &filesize);
            fseek(fd, 0, SEEK_SET);
        }

        sprintf(b, headers, mime, filename, filesize);

        if (fd)
        {

            // Wyślij nagłówek:
            ret = send(client_socket, b, strlen(b), 0);
            if (ret < 0)
            {
                perror("send()");
                close(client_socket);
                return NULL;
            }

            // Wysyłanie pliku:
            while ((ret = read(fileno(fd), b, 1024)) > 0)
            {
                ret = send(client_socket, b, ret, 0);
                if (ret < 0)
                {
                    perror("send()");
                    close(client_socket);
                    return NULL;
                }
            }
            fclose(fd);
        }
    }
    else
    {
        // Nagłówki:
        headers = "HTTP/1.1 200 OK\n"
                  "Server: Laboratoria PUS 01\n"
                  "Cache-Control: no-store, no-cache, must-revalidate\n"
                  "Keep-Alive: timeout=15, max=100\n"
                  "Connection: Keep-Alive\n"
                  "Content-Type: text/html; charset=utf-8\n"
                  "Content-Length: %d\n\r\n";

        content = html;

        sprintf(b, headers, strlen(content));
        sprintf(b, "%s%s", b, content);

        ret = send(client_socket, b, strlen(b), 0);
        if (ret < 0)
        {
            perror("send()");
            close(client_socket);
            return NULL;
        }
    }
    close(client_socket);
    pthread_exit(0);
    return NULL;
}

char *getFilename(char *fileName, char *fullPath)
{
    char *start = strstr(fullPath, "GET /");
    if (start != NULL)
    {
        start += strlen("GET /");
        char *end = strstr(start, " ");
        int len = strlen(start) - strlen(end);
        strncpy(fileName, start, len);
        fileName[len] = '\0';
        return fileName;
    }
    return NULL;
}