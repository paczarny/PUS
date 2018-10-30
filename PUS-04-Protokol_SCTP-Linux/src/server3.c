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
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/sctp.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define BUFF_SIZE 256

int main(int argc, char** argv) {

    int                     listenfd;
    int                     bytes;
    struct sockaddr_in      servaddr;
    char                    buffer[BUFF_SIZE];
    int                     stream_number;
    int		            user_arg;//0 - no increment or 1 - stream_number++
    struct sctp_initmsg     initmsg;
    struct sctp_sndrcvinfo  rcvinfo;
    struct sockaddr_in      clientaddr;
    struct sctp_event_subscribe events;


    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <PORT NUMBER> %s <STREAM NUMBER>\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }


    user_arg=atoi(argv[2]);
    if(!(user_arg==0 || user_arg==1)){
	fprintf(stderr, "BAD STREAM NUMBER, PLEASE TYPE 0 OR 1");
	exit(EXIT_FAILURE);
    }


    listenfd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    if (listenfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&events, 0, sizeof(events));
    memset(&clientaddr, 0, sizeof(struct sockaddr_in));
    memset(&rcvinfo, 0, sizeof(struct sctp_sndrcvinfo));
    memset(&initmsg, 0, sizeof(struct sctp_initmsg));
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family             =       AF_INET;
    servaddr.sin_port               =       htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr        =       htonl(INADDR_ANY);
    initmsg.sinit_num_ostreams = 10;
    initmsg.sinit_max_instreams = 10;
    initmsg.sinit_max_attempts = 5;
    initmsg.sinit_max_init_timeo = 60000;
    events.sctp_data_io_event = 1;

    if(setsockopt(listenfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(struct sctp_initmsg))==-1){
        perror("socket option");
        exit(EXIT_FAILURE);
    }


    if(setsockopt(listenfd, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events))==-1){
        perror("socket option");
        exit(EXIT_FAILURE);
    }



    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 5) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    for (;;) {
	memset(buffer,0, 256);
	printf("czekanie na msg\n");
        socklen_t fromlen=sizeof(clientaddr);
        bytes = sctp_recvmsg(listenfd, (void*)buffer, BUFF_SIZE, (struct sockaddr *)&clientaddr, &fromlen, &rcvinfo, 0);
        if (bytes == -1) {
            perror("recv()");
            exit(EXIT_FAILURE);
        }else if(bytes==0){
	    close(listenfd);
	    exit(EXIT_SUCCESS);
	}

	printf("wysylanie wiadomosci msg\n");

	switch(user_arg){
	case 0:
	if (sctp_sendmsg(listenfd, &buffer, sizeof(buffer),(struct sockaddr *)&clientaddr,fromlen, 0, 0, 0, 0, 0) == -1) {
            perror("send()");
            exit(EXIT_FAILURE);
        }
	break;
	case 1:
	stream_number=rcvinfo.sinfo_stream+1;
	if (sctp_sendmsg(listenfd, &buffer, sizeof(buffer),(struct sockaddr *)&clientaddr,fromlen, 0, 0, (uint16_t)stream_number , 0, 0) == -1) {
            perror("send()");
            exit(EXIT_FAILURE);
        }
	break;
	}

    }

}
