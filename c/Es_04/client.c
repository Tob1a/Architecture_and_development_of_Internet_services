/*
    chmod +x CLIENT
    gcc -c rxb.c
    gcc -c utils.c
    gcc -c client.c
    gcc rxb.o utils.o client.o -o CLIENT
*/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>  //serve per la socket e per la struct
#include <string.h> //per la memset
#include <unistd.h> //per la close()

#include "rxb.h"
#define MAX_REQUEST_SIZE 4096 // limite di trasmissione TCP
#include "utils.h"
/* Nel caso avessi delle stringhe UTF-8 */
#ifdef HAVE_LIBUNISTRING
#include <unistr.h>
#endif

int main(int argc, char *argv[])
{

    // argv[1] e' la porta argv[2] e' l'indirizzo;
    int err, sd;                       // err è il mio puntatore alla socket
    struct addrinfo hints, *res, *ptr; // hints è la mia struttura dove definisco la tipologia mentre res è il puntatore al primo elemento della lista di struct per la connessione. ptr uso per il for
    char mese[12], categoria[30], numero[30];

    if (argc != 3)
    {
        fprintf(stderr, "Errore numero di input\n");
        exit(EXIT_FAILURE);
    }

    // Inizializzazione di hints
    memset(&hints, 0, sizeof(hints)); // allora inizializzo la hints passandogli il puntatore iniziale. Poi inizializzo a (char)0 e il numero di byte da inizializzare
    hints.ai_family = AF_UNSPEC;      // gli indico che non so se ho ipv4 o ipv6
    hints.ai_socktype = SOCK_STREAM;  // gli indico che ho una connessione TCP

    // Ok ora devo aprire la socket
    err = getaddrinfo(argv[1], argv[2], &hints, &res); // err e' un intero che ha uno stato di come e' andata la funzione
    // getaddrinfo mi restituisce una lista struct per possibili connessioni
    if (err)
    {
        fprintf(stderr, "Errore getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        // ok ora provo a creare una socket
        sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); // ai_protocol è un protocollo specifico di default se ne occupa il sistema operativo
        // mi ritorna il file descriptor della socket come intero
        if (sd < 0)
        {
            continue; // vai alla prossima iterazione del ciclo
        }

        // la socket ha avuto successo tento la connessione
        err = connect(sd, ptr->ai_addr, ptr->ai_addrlen);
        // addr contiene l'indirizzo di destinazione a cui ci si vuole connettere
        // addrlen indica quanto e' lunga addr
        // connect mi restituisce un intero se e' 0 la connessione e' andata a buon fine. se ho -1 ho un errore
        if (err == 0)
        {
            break;
        }

        // In questo caso devo chiudere la socket. La chiudo perchè ho dovuto effetturare una connect senno' posso farne a meno
        close(sd);
    }
    if (ptr == NULL) // nel caso in cui ho finito di scorrere la lista
    {
        fputs("Errore di connessione", stderr);
        exit(EXIT_FAILURE);
    }

    // ok adesso non mi serve più res. Rimuovo la lista dalla memoria
    freeaddrinfo(res);

    rxb_t rxb; // questa variabile e' la strcut implementata dal prof come buffer

    // inizializza la struttura rxb
    rxb_init(&rxb, MAX_REQUEST_SIZE * 2);

    while (1)
    {
        printf("\nInserisci il mese:\n");
        fgets(mese, sizeof(mese), stdin); // usa fgets non scanf per evitare il buffered overflow
        if (!strcmp(mese, "fine\n"))      // devi mettere il \n senno non capisce più con fgets
        {
            printf("Termino\n");
            break;
        }
        printf("Inserisci la categoria:\n");
        fgets(categoria, sizeof(categoria), stdin);
        if (!strcmp(categoria, "fine\n"))
        {
            printf("Termino\n");
            break;
        }
        printf("Inserisci il numero':\n");
        fgets(numero, sizeof(numero), stdin);
        if (!strcmp(numero, "fine\n"))
        {
            printf("Termino\n");
            break;
        }

        err = write_all(sd, mese, strlen(mese)); // il write_all scrive sulla sd la stringa mese
        // quando uso questo comando la trasmette
        if (err < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        err = write_all(sd, categoria, strlen(categoria));
        if (err < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        err = write_all(sd, numero, strlen(numero));
        if (err < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // eseguo la read_all che non ho -.- . Meglio farla cosí
        while (1)
        {
            char line[MAX_REQUEST_SIZE]; // setto un buffered
            size_t line_len;

            // SA_RESTART è un segnale che mi permette di cap
            memset(line, 0, sizeof(line));
            line_len = sizeof(line) - 1;                   // per il terminatore
            err = rxb_readline(&rxb, sd, line, &line_len); // mi legge dalla socket e mi popula la struct rxb e copia l'output da rxb a line
            if (err < 0)
            {
                fputs("Errore rxb_readline!", stderr);
                rxb_destroy(&rxb); // mi libera l'allocamento di rxb
                close(sd);
                exit(EXIT_FAILURE);
            }

            /* Non e' obligatorio */
            /* Controlla se la stringa è di tipo UTF-8 */
#ifdef HAVE_LIBUNISTRING
            if (u8_check((uint8_t *)line, line_len) != NULL)
            {
                fputs("Errore: testo UTF-8 invalido!", stderr);
                rxb_destroy(&rxb);
                close(sd);
                exit(EXIT_FAILURE);
            }
#endif

            if (strcmp(line, "--- END REQUEST ---") == 0)
                break; // se il serve mi dice basta allora BASTA

            printf("%s", line);
        }
    }

    /* Cleanup */
    rxb_destroy(&rxb);
    close(sd);

    return EXIT_SUCCESS;
}