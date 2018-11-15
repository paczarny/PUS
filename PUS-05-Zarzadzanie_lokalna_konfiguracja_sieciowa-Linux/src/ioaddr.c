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

/*
previous ip settings command on Linux:
"service network-manager restart"
*/
int main(int argc, char **argv)
{

    int sockfd, retval;
    struct ifreq ifr;

    if (!((argc==5) || (argc==3)))
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

	if(strcmp(argv[2],"down")==0){
	    

	}
	else if(strcmp(argv[2],"up")==0){
	    strcpy(ifr.ifr_name, argv[1]);
	    ifr.ifr_addr.sa_family = AF_INET;
	    retval = inet_pton(AF_INET, argv[3], &ifr.ifr_addr.sa_data);
	    if(retval == 0){
		        fprintf(stderr, "inet_pton(): invalid network adddr");
		        exit(EXIT_FAILURE);
	    }

	    retval = ioctl(sockfd, SIOCSIFADDR, &ifr);
	    if (retval == -1) {
		perror("ioctl()addr");
		exit(EXIT_FAILURE);
	    }


	    ifr.ifr_netmask.sa_family = AF_INET;
	    retval = inet_pton(AF_INET, argv[4], &ifr.ifr_netmask.sa_data);
	    if(retval == 0){
                        fprintf(stderr, "inet_pton(): invalid network adddr");
                        exit(EXIT_FAILURE);
            }

	    retval = ioctl(sockfd, SIOCSIFNETMASK , &ifr);
	    if (retval == -1) {
		perror("ioctl()mask");
		exit(EXIT_FAILURE);
	    }
	}else{
		printf("Wrong method(use down or up)\n");
	}

    close(sockfd);
    exit(EXIT_SUCCESS);
}


