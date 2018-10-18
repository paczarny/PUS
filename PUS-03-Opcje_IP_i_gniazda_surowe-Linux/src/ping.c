/*
 * Data:                2009-02-27
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc ssrr.c -o ssrr
 * Uruchamianie:        $ ./ssrr <adres IP lub nazwa domenowa>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "checksum.h"

#define BUFSIZE 1500
char recvbuf[BUFSIZE];
char sendbuf [BUFSIZE];
//liczba bajtow za nagłowkiem icmp
int datalen;

//funkcje
void child();
void parent();


    /*Naglowek ICMP: */
    struct icmphdr          icmp_header = {0};

    /*Struktura zawierajaca wskazowki dla funkcji getaddrinfo(): */
    struct addrinfo         hints;

    /*Wskaznik na liste zwracana przez getaddrinfo()*/
    struct addrinfo          *result;

    int                     sockfd; /* Deskryptor gniazda. */
    int                     childSockfd; /* Deskryptor gniazda potomka. */
    int                     retval; /* Wartosc zwracana przez funkcje. */
    int                     ttl = 255;
    //do pętli pinga od 1 do 4
    short int               ping_req = 1;




int main(int argc, char** argv) {

    
    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <HOSTNAME OR IP ADDRESS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Wskazowki dla getaddrinfo(): */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family         =       AF_INET; /* Domena komunikacyjna (IPv4). */
    hints.ai_socktype       =       SOCK_RAW; /* Typ gniazda. */
    hints.ai_protocol       =       IPPROTO_ICMP; /* Protokol. */


    /* Pierwszy argument to adres IP lub nazwa domenowa: */
    retval = getaddrinfo(argv[1], NULL, &hints, &result);
    if (retval != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }


    if(fork())
        parent();
    else
        child();

 
    exit(EXIT_SUCCESS);
}



void parent()
{
	ttl=200;
        //proces macierzysty
        /* Utworzenie gniazda dla protokolu ICMP: */
        sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sockfd == -1) {
            perror("socket()");
        }

        /* Ustawienie opcji IP: */
        retval = setsockopt(
                     sockfd, IPPROTO_IP, IP_TTL,
                     &ttl, sizeof(ttl)
                 );

        if (retval == -1) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        }

    /* Jezeli lista jest pusta (nie utworzono gniazda): */
    if (result == NULL) {
        fprintf(stderr, "Client failure: could not create socket.\n");
        exit(EXIT_FAILURE);
    }

    /* Wypelnienie pol naglowka ICMP Echo: */
    srand(time(NULL));
    /* Typ komunikatu: */
    icmp_header.type                =       ICMP_ECHO;
    /* Kod komunikatu: */
    icmp_header.code                =       0;
    /* Identyfikator: */
    icmp_header.un.echo.id          =       htons(getpid());
    


    while(ping_req<5){
    /* Numer sekwencyjny: */
    icmp_header.un.echo.sequence    =       htons(ping_req);
    /* Suma kontrolna (plik checksum.h): */
    icmp_header.checksum            =       internet_checksum((unsigned short *)&icmp_header, sizeof(icmp_header));

     

    fprintf(stdout, "Sending ICMP Echo request...\n");
    /* Wyslanie komunikatu ICMP Echo: */
    retval = sendto(
                 sockfd,
                 &icmp_header, sizeof(icmp_header),
                 0,
                 result->ai_addr, result->ai_addrlen
             );

    if (retval == -1) {
        perror("sentdo()");
    }
	++ping_req;
	sleep(2);
    }


    /* Zwalniamy liste zaalokowana przez funkcje getaddrinfo(): */
    freeaddrinfo(result);

    close(sockfd); 

}




void child()
{
//proces potomny



    /* Utworzenie gniazda dla protokolu ICMP: */
    /* Utworzenie gniazda dla protokolu ICMP: */
        childSockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (childSockfd == -1) {
            perror("socket()");
        }

        /* Ustawienie opcji IP: */
        retval = setsockopt(
                     childSockfd, IPPROTO_IP, IP_TTL,
                     &ttl, sizeof(ttl)
                 );

        if (retval == -1) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        }

    /* Jezeli lista jest pusta (nie utworzono gniazda): */
    if (result == NULL) {
        fprintf(stderr, "Client failure: could not create socket.\n");
        exit(EXIT_FAILURE);
    }

    /* Wypelnienie pol naglowka ICMP Echo: */
    srand(time(NULL));
    /* Typ komunikatu: */
    icmp_header.type                =       8;
    /* Kod komunikatu: */
    icmp_header.code                =       0;
    /* Identyfikator: */
    icmp_header.un.echo.id          =       htons(getpid());
    /* Numer sekwencyjny: */
    icmp_header.un.echo.sequence    =       htons((unsigned short)rand());
    /* Suma kontrolna (plik checksum.h): */
    icmp_header.checksum            =       internet_checksum(
                                                (unsigned short *)&icmp_header,
                                                sizeof(icmp_header)
                                            );

	
    while(1){
    /* Wyslanie komunikatu ICMP Echo: */
    socklen_t len = result->ai_addrlen;
    retval = recvfrom(
                 childSockfd,
                 recvbuf, sizeof(recvbuf),
                 0,
                 result->ai_addr, &len
             );

    if (retval == -1) {
        perror("sentdo()");
    }

     
    }
}
