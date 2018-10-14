/*
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

int main(int argc, char **argv)
{

    //zbiory deskryptorow
    fd_set set_of_decriptors, test_descriptors;

    /* Deskryptory dla serwera u klienta: */
    int server_socket, client_socket;

    int retval; /* Wartosc zwracana przez funkcje. */

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    int client_addr_len, server_addr_len;

    /* Bufor wykorzystywany przez write() i read(): */
    char buff[256];

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char addr_buff[256];

    //max liczba klientow
    static int MAX_CLI = 10;

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
    if (listen(server_socket, MAX_CLI) == -1)
    {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    //wyzerowanie struktury z deskryptorami
    FD_ZERO(&set_of_decriptors);
    //dodanie deskryptora serwera do struktory
    FD_SET(server_socket, &set_of_decriptors);

    //do sprawdzenie kiedy klient zakończy połączenie
    int client_data = 1;

    while (1)
    {

        fprintf(stdout, "Server is listening for incoming connection...\n");

        //kopia dekryptorow
        test_descriptors = set_of_decriptors;

        //czekanie na żądanie od klienta
        retval = select(FD_SETSIZE, &test_descriptors, (fd_set *)0, (fd_set *)0, (struct timeval *)0);
        if (retval < 1)
        {
            perror("select()");
            exit(1);
        }

        //kiedy bedzie aktywny jakis dekryptor to sprawdzamy po kolei przez FD_ISSET
        int i;
        for (i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &test_descriptors))
            {
                //jesli aktywnosc jest zwiazana z gniazdem serwera tzn ze klient czeka na polaczenie
                if (i == server_socket)
                {
                    client_addr_len = sizeof(client_addr);
                    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
                    if (client_socket == -1)
                    {
                        perror("accept()");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_socket, &set_of_decriptors);
                    fprintf(
                        stdout, "TCP connection accepted from %s:%d\n",
                        inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
                        ntohs(client_addr.sin_port));
                }
                else
                {
                    //obsluga klienta

                    ioctl(i, FIONREAD, &client_data);
                    if (client_data == 0)
                    {
                        close(i);
                        FD_CLR(i, &set_of_decriptors);
                        fprintf(stdout, "Usuwam klienta na dp %d..\n", i);
                    }
                    else
                    {
                        memset(buff,0,sizeof buff);
                        retval = recv(i, buff, sizeof(buff), 0);
                        if (retval == -1)
                        {
                            perror("recv()");
                            exit(EXIT_FAILURE);
                        }
                        // buff[retval] = '\0';
                        int j;
                        for (j = 0; j < FD_SETSIZE; j++)
                        {
                            if (FD_ISSET(j, &set_of_decriptors) && j != server_socket)
                            {
                                retval = send(j, buff, sizeof(buff), 0);
                                if (retval == -1)
                                {
                                    perror("send()");
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        printf("Massage send to others\n");
                    }
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}
