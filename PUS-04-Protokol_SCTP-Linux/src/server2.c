/*
 * Data:                2009-03-01
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc server1.c -o server1
 * Uruchamianie:        $ ./server1 <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define BUFF_SIZE 256

int main(int argc, char **argv)
{

    int listenfd, connfd;
    int retval;
    struct sockaddr_in servaddr;
    char buffer[BUFF_SIZE];
    struct sctp_initmsg initmsg;

    if (argc != 2)
    {
        fprintf(stderr, "Invocation: %s <PORT NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (listenfd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&initmsg, 0, sizeof(initmsg));
    initmsg.sinit_num_ostreams = 2;
    initmsg.sinit_max_instreams = 2;
    retval = setsockopt(listenfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg));
    if (retval == -1)
    {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 5) == -1)
    {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        connfd = accept(listenfd, NULL, 0);
        if (connfd == -1)
        {
            perror("accept()");
            exit(EXIT_FAILURE);
        }
        printf("Client connected\n");

        // Pobranie daty:
        time_t time_s;
        time(&time_s);
        struct tm *tm_s = localtime(&time_s);

        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "DATA: %d.%d.%d", tm_s->tm_mday, tm_s->tm_mon + 1, tm_s->tm_year + 1900);
        retval = sctp_sendmsg(connfd, buffer, (size_t)strlen(buffer), NULL, 0, 0, 0, 0, 0, 0);
        if (retval == -1)
        {
            perror("sctp_sendmsg()");
            exit(EXIT_FAILURE);
        }

        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "CZAS: %d:%d:%d", tm_s->tm_hour, tm_s->tm_min, tm_s->tm_sec);
        retval = sctp_sendmsg(connfd, buffer, (size_t)strlen(buffer), NULL, 0, 0, 0, 1, 0, 0);
        if (retval == -1)
        {
            perror("sctp_sendmsg()");
            exit(EXIT_FAILURE);
        }
    }
}
