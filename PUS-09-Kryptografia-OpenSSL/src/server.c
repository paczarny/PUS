/*
 * Data:                2009-02-10
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc server.c -o server
 * Uruchamianie:        $ ./server <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h> 
void get_hmac(char message[], int message_len, unsigned char digest[]);
void encrypt_message(char message[], char plaintext[]);

int main(int argc, char** argv) {

    char            plaintext[80];
    int             sockfd; /* Deskryptor gniazda. */
    int             retval; /* Wartosc zwracana przez funkcje. */
    char            message[256];
    unsigned char digest[EVP_MAX_MD_SIZE];
    int             i;

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor wykorzystywany przez recvfrom(): */
    char            buff[256];

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            addr_buff[256];


    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu UDP: */
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej serwera: */
    memset(&server_addr, 0, sizeof(server_addr));
    /* Domena komunikacyjna (rodzina protokolow): */
    server_addr.sin_family          =       AF_INET;
    /* Adres nieokreslony (ang. wildcard address): */
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);
    /* Numer portu: */
    server_addr.sin_port            =       htons(atoi(argv[1]));
    /* Rozmiar struktury adresowej serwera w bajtach: */
    server_addr_len                 =       sizeof(server_addr);

    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(sockfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is waiting for UDP datagram...\n");

    client_addr_len = sizeof(client_addr);

    /* Oczekiwanie na dane od klienta: */
    retval = recvfrom(
                 sockfd,
                 buff, sizeof(buff),
                 0,
                 (struct sockaddr*)&client_addr, &client_addr_len
             );
    if (retval == -1) {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "UDP datagram received from %s:%d.\n",
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
            ntohs(client_addr.sin_port)
           );


    encrypt_message(buff, plaintext);
    i=16;
    while(plaintext[i]!='\0')
    {
        message[i-16]=plaintext[i];
        i++;
    }
    message[i-16]='\0';

    get_hmac(message, strlen(message), digest);


    fprintf(stdout, "\nKlucz uwierzytelniający (hex): ");
    for (i = 0; i < 10; i++) {
        fprintf(stdout, "%02x", digest[i]);
    }

    fprintf(stdout, "\nKlucz uwierzytelniający (hex): ");
    for (i = 0; i < 5; i++) {
        fprintf(stdout, "%02x", plaintext[i]);
    }


    close(sockfd);
    exit(EXIT_SUCCESS);
}



void encrypt_message(char ciphertext[], char plaintext[])
{
        /* Wartosc zwracana przez funkcje: */
    int retval;

    int tmp;

    /* Rozmiar tekstu i szyfrogramu: */
    int plaintext_len, ciphertext_len;

    ciphertext_len=48;



    /*
     * == 0 - padding PKCS nie bedzie stosowany
     * != 0 - padding PKCS bedzie stosowany
     */
    int padding=1;

    /* Klucz i wektor inicjalizacyjny sa stalymi, aby wyniki byly przewidywalne. */

    /* Klucz: */
    unsigned char key[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
                           0x00,0x01,0x02,0x03,0x04,0x05
                          };

    /* Kontekst: */
    EVP_CIPHER_CTX *ctx;

    const EVP_CIPHER* cipher;



    /* Zaladowanie tekstowych opisow bledow: */
    ERR_load_crypto_strings();

    /* Alokacja pamieci dla kontekstu: */
    ctx = EVP_CIPHER_CTX_new();
    /* Inicjalizacja kontekstu: */
    EVP_CIPHER_CTX_init(ctx);

    /*
     * Parametry algorytmu AES dla trybu ECB i klucza o rozmiarze 128-bitow.
     * Liste funkcji typu "EVP_aes_128_ecb()" mozna uzyskac z pliku <openssl/evp.h>.
     * Strony podrecznika systemowego nie sa kompletne.
     */
    cipher = EVP_aes_128_ecb();
    EVP_CIPHER_block_size(cipher);

    fprintf(stdout, "Decrypting...\n\n");
    /* Konfiguracja kontekstu dla odszyfrowywania: */
    retval = EVP_DecryptInit_ex(ctx, cipher, NULL, key, NULL);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /*
     * Domyslnie OpenSSL stosuje padding. 0 - padding nie bedzie stosowany.
     * Funkcja moze byc wywolana tylko po konfiguracji kontekstu
     * dla szyfrowania/deszyfrowania (odzielnie dla kazdej operacji).
     */
    EVP_CIPHER_CTX_set_padding(ctx, padding);

    /* Odszyfrowywanie: */
    retval = EVP_DecryptUpdate(ctx, (unsigned char*)plaintext, &plaintext_len,
                               (const unsigned char *)ciphertext, ciphertext_len);

    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /*
     * Prosze zwrocic uwage, ze rozmiar bufora 'plaintext' musi byc co najmniej o
     * rozmiar bloku wiekszy od dlugosci szyfrogramu (na padding):
     */
    retval = EVP_DecryptFinal_ex(ctx, (unsigned char*)plaintext + plaintext_len, &tmp);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    plaintext_len += tmp;
    plaintext[plaintext_len] = '\0';

    EVP_CIPHER_CTX_cleanup(ctx);
    if (ctx) {
        free(ctx);
    }
    /* Zwolnienie tekstowych opisow bledow: */
    ERR_free_strings();
}


void get_hmac(char message[], int message_len, unsigned char digest[]){
    /* Wartosc zwracana przez funkcje: */
    int retval;

    /* Rozmiar tekstu i szyfrogramu: */
    unsigned int  digest_len;

    /* Kontekst: */
    HMAC_CTX *ctx_hmac;
    const EVP_MD* md;
    
    unsigned char key[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
                           0x00,0x01,0x02,0x03,0x04,0x05
                          };

    /* Zaladowanie tekstowych opisow bledow: */
    ERR_load_crypto_strings();

    /*
     * Zaladowanie nazw funkcji skrotu do pamieci.
     * Wymagane przez EVP_get_digestbyname():
     */
    OpenSSL_add_all_digests();

    md = EVP_get_digestbyname("md5");
    if (!md) {
        fprintf(stderr, "Unknown message digest: %s\n", "md5");
        exit(EXIT_FAILURE);
    }
    /* Alokacja pamieci dla kontekstu: */
    ctx_hmac = (HMAC_CTX*)malloc(sizeof(HMAC_CTX));

    /* Inicjalizacja kontekstu: */
    HMAC_CTX_init(ctx_hmac);

    /* Konfiguracja kontekstu: */
    retval = HMAC_Init_ex(ctx_hmac, key, sizeof(key), md, NULL);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    
    retval = HMAC_Update(ctx_hmac,(const unsigned char *)message, message_len);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    
    retval = HMAC_Final(ctx_hmac, digest, &digest_len);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    /*
     * Usuwa wszystkie informacje z kontekstu i zwalnia pamiec zwiazana
     * z kontekstem:
     */

    /* Usuniecie nazw funkcji skrotu z pamieci. */
    HMAC_cleanup(ctx_hmac);

    
    /* Zwolnienie tekstowych opisow bledow: */
    ERR_free_strings();



}