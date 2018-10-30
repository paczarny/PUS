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
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define BUFF_SIZE 256

int main(int argc, char** argv) {

    int                     sockfd;
    int                     retval, bytes;
    char                    buff[BUFF_SIZE];
    char                    *retptr;
    struct sctp_initmsg     initmsg;
    struct sctp_sndrcvinfo  rcvinfo;
    struct sockaddr_in      serveraddr;
    struct sctp_event_subscribe events;

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IP ADDRESS> <PORT NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&events, 0, sizeof(events));
    memset(&serveraddr, 0, sizeof(struct sockaddr_in));
    memset(&rcvinfo, 0, sizeof(struct sctp_sndrcvinfo));
    memset(&initmsg, 0, sizeof(struct sctp_initmsg));
    initmsg.sinit_num_ostreams = 10;
    initmsg.sinit_max_instreams = 9;
    initmsg.sinit_max_attempts = 5;
    initmsg.sinit_max_init_timeo=60000;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[2]));
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    retval = inet_pton(AF_INET, argv[1], &serveraddr.sin_addr);
    events.sctp_data_io_event = 1;

    if (retval == 0) {
        fprintf(stderr, "inet_pton(): invalid network address!\n");
        exit(EXIT_FAILURE);
    } else if (retval == -1) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }


    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(struct sctp_initmsg))==-1){
	perror("socket option");
	exit(EXIT_FAILURE);
    }

    if(setsockopt(sockfd, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events))==-1){
        perror("socket option");
        exit(EXIT_FAILURE);
    }




    int number_of_stream = 0;

    for (;;) {
	fprintf(stderr, "Client stream number %d\n",number_of_stream);

        memset(buff,0, 256);
        retptr = fgets(buff, BUFF_SIZE, stdin);
        if ((retptr == NULL) || (strcmp(buff, "\n") == 0)) {
            break;
        }

        bytes = sctp_sendmsg(sockfd, &buff, sizeof(buff), (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in), 0, 0, number_of_stream, 0, 0);
	if (bytes == -1) {
            perror("send()");
            exit(EXIT_FAILURE);
        }



        bytes = sctp_recvmsg(sockfd, (void*)buff, BUFF_SIZE,NULL, NULL,&rcvinfo, NULL);
        if (bytes == -1) {
            perror("recv()");
            exit(EXIT_FAILURE);
        }

        fprintf(stderr, "Message received, stream number %d\n", rcvinfo.sinfo_stream);
	fprintf(stderr, "Association ID  %d\n",rcvinfo.sinfo_assoc_id);
	fprintf(stderr, "SSN  %d\n",rcvinfo.sinfo_ssn);


        fprintf(stdout, "Message: ");
        fflush(stdout);
        retval = write(STDOUT_FILENO, buff, bytes);
        //server stream equals client stream
        number_of_stream = rcvinfo.sinfo_stream;
    }

    close(sockfd);
    exit(EXIT_SUCCESS);
}
