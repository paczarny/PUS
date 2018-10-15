/*
 * Data:                2009-02-10
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc client2.c -o client2
 * Uruchamianie:        $ ./client2 <adres IP> <numer portu> <wiadomosc>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>		/* close() */
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <sys/types.h>
#include <netdb.h> /* getaddrinfo() */

struct addrinfo *get_addrinfo(char *ip, char *port);

int main(int argc, char **argv)
{

	int sockfd;					  /* Desktryptor gniazda. */
	int retval;					  /* Wartosc zwracana przez funkcje. */
	struct sockaddr *remote_addr; /* Gniazdowa struktura adresowa. */
	socklen_t addr_len;			  /* Rozmiar struktury w bajtach. */
	char buff[256];				  /* Bufor dla funkcji recvfrom(). */
	struct addrinfo *addr_info;

	if (argc != 4)
	{
		fprintf(
			stderr,
			"Invocation: %s <IPv4 ADDRESS> <PORT> <INTERFACE>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	addr_info = get_addrinfo(argv[1], argv[2]);
	if (addr_info == NULL)
	{
		fprintf(stderr, "Cant get valid addrinfo\n");
		exit(EXIT_FAILURE);
	}

	/* Utworzenie gniazda dla protokolu TCP: */
	sockfd = socket(addr_info->ai_family, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	remote_addr = addr_info->ai_addr;

	// remote_addr.sin6_port = htons(atoi(argv[2])); /* Numer portu. */
	addr_len = addr_info->ai_addrlen; /* Rozmiar struktury adresowej w bajtach. */

	connect(sockfd, remote_addr, addr_len);

	/* Oczekiwanie na odpowiedz. */
	retval = recv(sockfd, buff, sizeof(buff), 0);
	if (retval == -1)
	{
		perror("recv()");
		exit(EXIT_FAILURE);
	}

	// buff[retval] = '\0';

	fprintf(stdout, "Server response: '%s'\n", buff);

	close(sockfd);
	exit(EXIT_SUCCESS);
}

struct addrinfo *get_addrinfo(char *ip, char *port)
{

	/* Struktura zawierajaca wskazowki dla funkcji getaddrinfo(): */
	struct addrinfo hints;

	/*
     * Wskaznik na liste zwracana przez getaddrinfo() oraz wskaznik uzywany do
     * poruszania sie po elementach listy:
     */
	struct addrinfo *result, *rp;

	/* Wartosc zwracana przez funkcje: */
	int retval;

	/* Rozmiar gniazdowej struktury adresowej: */
	// socklen_t sockaddr_size;

	memset(&hints, 0, sizeof(hints));
	/* Pozwalamy na AF_INET or AF_INET6: */
	hints.ai_family = AF_UNSPEC;
	/* Gniazdo typu SOCK_STREAM (TCP): */
	hints.ai_socktype = SOCK_STREAM;
	/* Dowolny protokol: */
	hints.ai_protocol = 0;

	if ((retval = getaddrinfo(ip, port, &hints, &result)) != 0)
	{
		fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
		exit(EXIT_FAILURE);
	}

	/* Przechodzimy kolejno przez elementy listy: */
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{

		/* Zapamietujemy wersje protokolu oraz rozmiar struktury adresowej: */
		if (rp->ai_family == AF_INET || rp->ai_family == AF_INET6)
		{
			return rp;
		}
	}

	return NULL;
}
