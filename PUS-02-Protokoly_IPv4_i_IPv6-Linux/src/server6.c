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

int main(int argc, char **argv)
{

    /* Deskryptory dla serwera i klienta: */
    int server_socket, client_socket;

    int retval; /* Wartosc zwracana przez funkcje. */

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t client_addr_len, server_addr_len;

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char addr_buff[256];

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

    /* Przeksztalcenie gniazda w gniazdo nasluchujace: */
    if (listen(server_socket, 100) == -1)
    {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    char* response = "Laboratorium PUS";
    while (1)
    {

        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1)
        {
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        fprintf(
            stdout, "TCP connection accepted from %s:%d\n",
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
            ntohs(client_addr.sin_port));

        retval = send(client_socket, response, strlen(response), 0);
        if (retval == -1)
        {
            perror("send()");
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}
