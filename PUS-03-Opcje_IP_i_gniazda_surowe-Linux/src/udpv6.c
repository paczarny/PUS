/*
 * Data:                2009-02-27
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc udp.c -o udp
 * Uruchamianie:        $ ./udp <adres IP lub nazwa domenowa> <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef __USE_BSD
#define __USE_BSD
#endif
#include <netinet/ip.h>
#define __FAVOR_BSD
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include "checksum.h"

#define SOURCE_PORT 5050
#define SOURCE_ADDRESS "192.0.2.1"

/* Struktura pseudo-naglowka (do obliczania sumy kontrolnej naglowka UDP): */
struct phdr {
    struct in_addr ip_src, ip_dst;
    unsigned char unused;
    unsigned char protocol;
    unsigned short length;

};

int main(int argc, char** argv) {

    int                     sockfd; /* Deskryptor gniazda. */
    int                     offset; /* Do ustawiania opcji gniazda. */
    int                     retval; /* Wartosc zwracana przez funkcje. */

    /* Struktura zawierajaca wskazowki dla funkcji getaddrinfo(): */
    struct addrinfo         hints;

    /* Nagłówek udp*/
    struct udphdr           udp_header = {0};


    /*
     * Wskaznik na liste zwracana przez getaddrinfo() oraz wskaznik uzywany do
     * poruszania sie po elementach listy:
     */
    struct addrinfo         *rp, *result;

   
    /* SPrawdzenie argumentow wywolania: */
    if (argc != 3) {
        fprintf(
            stderr,
            "Invocation: %s <HOSTNAME OR IP ADDRESS> <PORT>\n",
            argv[0]
        );

        exit(EXIT_FAILURE);
    }

    /* Wskazowki dla getaddrinfo(): */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family         =       PF_INET6; /* Domena komunikacyjna (IPv4). */
    hints.ai_socktype       =       SOCK_RAW; /* Typ gniazda. */
    hints.ai_protocol       =       IPPROTO_UDP; /* Protokol. */


    retval = getaddrinfo(argv[1], NULL, &hints, &result);
    if (retval != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }

    /* Opcja okreslona w wywolaniu setsockopt() zostanie wlaczona: */
    offset = 6;

    /* Przechodzimy kolejno przez elementy listy: */
    for (rp = result; rp != NULL; rp = rp->ai_next) {

        /* Utworzenie gniazda dla protokolu UDP: */
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            perror("socket()");
            continue;
        }

        /* Ustawienie opcji IP_HDRINCL: */
        retval = setsockopt(
                     sockfd,
                     IPPROTO_IPV6, IPV6_CHECKSUM,
                     &offset, sizeof(int)
                 );

        if (retval == -1) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        } else {
            /* Jezeli gniazdo zostalo poprawnie utworzone i
             * opcja IP_HDRINCL ustawiona: */
            break;
        }
    }

    /* Jezeli lista jest pusta (nie utworzono gniazda): */
    if (rp == NULL) {
        fprintf(stderr, "Client failure: could not create socket.\n");
        exit(EXIT_FAILURE);
    }

    /*********************************/
    /* Wypelnienie pol naglowka UDP: */
    /*********************************/

    /* Port zrodlowy: */
    udp_header.uh_sport            =       htons(SOURCE_PORT);
    /* Port docelowy (z argumentu wywolania): */
    udp_header.uh_dport            =       htons(atoi(argv[2]));

    /* Rozmiar naglowka UDP i danych. W tym przypadku tylko naglowka: */
    udp_header.uh_ulen             =       htons(sizeof(struct udphdr));

    fprintf(stdout, "Sending UDP...\n");

    /* Wysylanie datagramow co 1 sekunde: */
    for (;;) {

        /*
         * Prosze zauwazyc, ze pseudo-naglowek nie jest wysylany
         * (ale jest umieszczony w buforze za naglowkiem UDP dla wygodnego
         * obliczania sumy kontrolnej):
         */
        retval = sendto(
                     sockfd,
                     &udp_header, sizeof(udp_header),
                     0,
                     rp->ai_addr, rp->ai_addrlen
                 );

        if (retval == -1) {
            perror("sendto()");
        }

        sleep(1);
    }

    exit(EXIT_SUCCESS);
}
