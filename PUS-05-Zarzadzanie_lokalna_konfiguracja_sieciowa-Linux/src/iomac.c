/*
 * Data:                2009-03-15
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc ioarp.c -o ioarp
 * Uruchamianie:        $ ./ioarp <adres IPv4> <adres MAC>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h> /* inet_pton() */
#include <net/if_arp.h>
#include <netinet/in.h> /* struct sockaddr_in */
#include <sys/ioctl.h>
#include <net/if.h>

void print_mac(int sockfd, struct ifreq ifr);
void print_mtu(int sockfd, struct ifreq ifr);

int main(int argc, char **argv)
{

    int sockfd, retval;
    struct arpreq request;
    struct ifreq ifr;

    if (argc != 4)
    {
        fprintf(stderr, "Invocation: %s <INTERFACE> <MAC ADDRESS> <MTU>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&ifr, 0, sizeof(struct ifreq));

    strcpy(ifr.ifr_name, argv[1]);

    print_mac(sockfd, ifr);
    print_mtu(sockfd, ifr);


    retval = ioctl(sockfd, SIOCGIFFLAGS, &ifr);
    if (retval == -1) {
        perror("ioctl()");
        exit(EXIT_FAILURE);
    }

    ifr.ifr_flags &= ~IFF_UP;
    retval = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
    if (retval == -1) {
        perror("ioctl()");
        exit(EXIT_FAILURE);
    }

    /* Adres MAC: */
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

    /* Ustawienie MAC */
    retval = sscanf(
        argv[2], "%2x:%2x:%2x:%2x:%2x:%2x",
        (unsigned int *)&ifr.ifr_hwaddr.sa_data[0],
        (unsigned int *)&ifr.ifr_hwaddr.sa_data[1],
        (unsigned int *)&ifr.ifr_hwaddr.sa_data[2],
        (unsigned int *)&ifr.ifr_hwaddr.sa_data[3],
        (unsigned int *)&ifr.ifr_hwaddr.sa_data[4],
        (unsigned int *)&ifr.ifr_hwaddr.sa_data[5]);

    if (retval != 6)
    {
        fprintf(stderr, "Invalid address format!\n");
        exit(EXIT_FAILURE);
    }

    retval = ioctl(sockfd, SIOCSIFHWADDR, &ifr);
    if (retval == -1)
    {
        perror("ioctl SIOCSIFHWADDR()");
        exit(EXIT_FAILURE);
    }

    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    retval = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
    if (retval == -1) {
        perror("ioctl()");
        exit(EXIT_FAILURE);
    }



    /* Ustawienie MTU */
    ifr.ifr_mtu = atoi(argv[3]);
    retval = ioctl(sockfd, SIOCSIFMTU, &ifr);
    if (retval == -1)
    {
        perror("ioctl SIOCSIFMTU()");
        exit(EXIT_FAILURE);
    }

    print_mac(sockfd, ifr);
    print_mtu(sockfd, ifr);

    // memset(&request, 0, sizeof(struct arpreq));

    close(sockfd);
    exit(EXIT_SUCCESS);
}

void print_mac(int sockfd, struct ifreq ifr)
{
    /* Pobranie MAC */
    int retval = ioctl(sockfd, SIOCGIFHWADDR, &ifr);
    if (retval == -1)
    {
        perror("ioctl()");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "MAC: %2x:%2x:%2x:%2x:%2x:%2x\n",
            (unsigned char)ifr.ifr_hwaddr.sa_data[0],
            (unsigned char)ifr.ifr_hwaddr.sa_data[1],
            (unsigned char)ifr.ifr_hwaddr.sa_data[2],
            (unsigned char)ifr.ifr_hwaddr.sa_data[3],
            (unsigned char)ifr.ifr_hwaddr.sa_data[4],
            (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
}

void print_mtu(int sockfd, struct ifreq ifr)
{
    /* Pobranie MTU */
    int retval = ioctl(sockfd, SIOCGIFMTU, &ifr);
    if (retval == -1)
    {
        perror("ioctl()ccc");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "MTU: %d\n", ifr.ifr_mtu);
}
