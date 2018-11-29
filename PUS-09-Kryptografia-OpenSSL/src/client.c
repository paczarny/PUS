/*
 * Data:                2009-02-10
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc client.c -o client
 * Uruchamianie:        $ ./client <adres IP> <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h> 

unsigned int get_hmac(char message[],int message_len,  unsigned char digest[]);
void set_cipherText(unsigned char statement[],int statement_len,  unsigned char ciphertext[]);

int main(int argc, char** argv) {
    
    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IPv4 ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    int             sockfd;                 /* Desktryptor gniazda. */
    int             retval;                 /* Wartosc zwracana przez funkcje. */
    struct          sockaddr_in remote_addr;/* Gniazdowa struktura adresowa. */
    socklen_t       addr_len;               /* Rozmiar struktury w bajtach. */

    char message[256] = "Laboratorium PUS.";
  
    unsigned int message_len = strlen(message);

    unsigned int digest_len;

    unsigned char digest[EVP_MAX_MD_SIZE];
    
    digest_len = get_hmac(message, message_len,digest);
    
    unsigned char statement[message_len+digest_len+1];
    unsigned int statement_len = message_len+digest_len+1;

    /* Bufor na szyfrogram: */
    unsigned char ciphertext[80];

    /*[HMAC+STATEMENT]*/
    for(int i=0;i<digest_len;i++)
        statement[i]=digest[i];
    
    for(int i=digest_len, j=0;i<statement_len-1;i++, j++)
        statement[i]=message[j];
    statement[statement_len-1]='\0';

    
    set_cipherText(statement,statement_len-1,ciphertext);


    /* Utworzenie gniazda dla protokolu UDP: */
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej dla adresu zdalnego (serwera): */
    memset(&remote_addr, 0, sizeof(remote_addr));
    /* Domena komunikacyjna (rodzina protokolow): */
    remote_addr.sin_family = AF_INET;

    /* Konwersja adresu IP z postaci kropkowo-dziesietnej: */
    retval = inet_pton(AF_INET, argv[1], &remote_addr.sin_addr);
    if (retval == 0) {
        fprintf(stderr, "inet_pton(): invalid network address!\n");
        exit(EXIT_FAILURE);
    } else if (retval == -1) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    remote_addr.sin_port = htons(atoi(argv[2])); /* Numer portu. */
    addr_len = sizeof(remote_addr); /* Rozmiar struktury adresowej w bajtach. */

    fprintf(stdout, "\nSending message to %s.\n", argv[1]);

    /* sendto() wysyla dane na adres okreslony przez strukture 'remote_addr': */
    retval = sendto(
                 sockfd,
                 ciphertext, strlen((const char *)ciphertext),
                 0,
                 (struct sockaddr*)&remote_addr, addr_len
             );

    if (retval == -1) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    exit(EXIT_SUCCESS);
}




unsigned int get_hmac(char message[], int message_len, unsigned char digest[]){
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
    fprintf(stdout, "Klucz uwierzytelniający (hex): ");
    for (int i = 0; i < 10; i++) {
        fprintf(stdout, "%02x", digest[i]);
    }
    
    /* Zwolnienie tekstowych opisow bledow: */
    ERR_free_strings();


    return digest_len;

}





void set_cipherText(unsigned char plaintext[], int plaintext_len,  unsigned char ciphertext[]){
/* Wartosc zwracana przez funkcje: */
    int retval;

    int i, tmp;


    /* Rozmiar tekstu i szyfrogramu: */
    int ciphertext_len;

    /* Rozmiar bloku, na ktorym operuje algorytm: */
    int block_size;

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

    block_size = EVP_CIPHER_block_size(cipher);


    retval = EVP_EncryptInit_ex(ctx, cipher, NULL, key, NULL);
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


    /* Szyfrowanie: */
    retval = EVP_EncryptUpdate(ctx, ciphertext, &ciphertext_len,
                               (unsigned char*)plaintext, plaintext_len);

    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    retval = EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_len, &tmp);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /*
     * Usuwa wszystkie informacje z kontekstu i zwalnia pamiec zwiazana
     * z kontekstem:
     */
    EVP_CIPHER_CTX_cleanup(ctx);

    ciphertext_len += tmp;


    fprintf(stdout, "\n\n");
    fprintf(stdout, "Plaintext length: %u\n", plaintext_len);
    fprintf(stdout, "Ciphertext length: %u\n", ciphertext_len);


    /* Zwolnienie tekstowych opisow bledow: */
    ERR_free_strings();

}