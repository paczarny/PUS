/*
 * Data:                2009-03-01
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc client1.c -o client1
 * Uruchamianie:        $ ./client1 <adres IP> <numer portu>
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

#define BUFF_SIZE 256

int main(int argc, char **argv)
{

    int sockfd;
    int retval, flags, slen;
    struct addrinfo hints, *result;
    char buff[BUFF_SIZE];
    struct sctp_initmsg initmsg;
    struct sctp_status s_status;
    struct sctp_sndrcvinfo s_sndrcvinfo;
    struct sctp_event_subscribe s_events;

    if (argc != 3)
    {
        fprintf(stderr, "Invocation: %s <IP ADDRESS> <PORT NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    retval = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (retval != 0)
    {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }

    if (result == NULL)
    {
        fprintf(stderr, "Could not connect!\n");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(result->ai_family, result->ai_socktype, IPPROTO_SCTP);
    if (sockfd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&initmsg, 0, sizeof(initmsg));
    initmsg.sinit_num_ostreams = 3;
    initmsg.sinit_max_instreams = 4;
    initmsg.sinit_max_attempts = 5;
    retval = setsockopt(sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg));
    if (retval != 0)
    {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, result->ai_addr, result->ai_addrlen) == -1)
    {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    memset(&s_events, 0, sizeof(s_events));
    s_events.sctp_data_io_event = 1;
    retval = setsockopt(sockfd, SOL_SCTP, SCTP_EVENTS,
                        (const void *)&s_events, sizeof(s_events));
    slen = sizeof(s_status);
    retval = getsockopt(sockfd, SOL_SCTP, SCTP_STATUS, &s_status, (socklen_t *)&slen);

    printf("ID          = %d\n", s_status.sstat_assoc_id);
    printf("STATE       = %d\n", s_status.sstat_state);
    printf("INSTREAMS   = %d\n", s_status.sstat_instrms);
    printf("OUTSTREAMS  = %d\n", s_status.sstat_outstrms);

    for (int i = 0; i < initmsg.sinit_num_ostreams - 1; i++)
    {
        memset(buff, 0, sizeof(buff));
        retval = sctp_recvmsg(sockfd, buff, BUFF_SIZE, NULL, 0, &s_sndrcvinfo, &flags);
        if (retval > 0)
        {
            buff[retval] = 0;
            printf("(STREAM: %d) %s\n", s_sndrcvinfo.sinfo_stream, buff);
            fflush(stdout);
        }
    }
    close(sockfd);
    exit(EXIT_SUCCESS);
}
