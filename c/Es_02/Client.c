// controllo_conto_corrente
#define _POSIX_C_SOURCE 200112L // definisce la struttura addrinfo
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include "rxb.h"

int main(int argc, char *argv[])
{
    char categoria[60];
    int sd, err;
    struct addrinfo hints, *ptr, *res; // 3 strutture addr info
    // ogni addrinfo ha un puntatore all'addrinfo successiva

    //  controllo degli argomenti
    if (argc > 4 || argc < 3)
    {
        fprintf(stderr, "Numero errato di argomenti\n");
        exit(EXIT_FAILURE);
    }

    // inizializzo la struttura hints
    memset(&hints, 0, sizeof(hints)); // setta sizeof(hits) byte di hints a 0

    // hits parametro della getadrinfo per cercare la lista di nodi alla quale è possibile effettuare una connessione

    // setto la struttura hints
    hints.ai_socktype = SOCK_STREAM; // metto la tipologia sockstream sulle hits
    hints.ai_protocol = AF_UNSPEC;   // gli metto qualsiasi tipologia di ipv 4 o 6

    // cerco i nodi disponibili per la connessione socket
    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (err != 0)
    {
        fprintf(stderr, "Fallita getaddrinfo\n");
        exit(EXIT_FAILURE);
    }

    // scorro tutti i nodi possibili per tentare una connessione
    // setti inizialmente ptr a res che è il puntatore iniziale della lista e poi scorrila finche non e' null
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        // socket
        sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sd < 0)
        {
            // non mi serve chiudere qua dentro la socket (close) perchè non è partita e quindi non ho bisogno di chiuderla
            continue; // passa al ciclo successivo
        }

        // la socket è andata, ora provo la connect
        err = connect(sd, ptr->ai_addr, ptr->ai_addrlen);
        if (err == 0)
        {
            break;
        }
        // in questo caso la connect non è andata e quindi devo per forza chiudere la socket
        close(sd);
    }

    // controllo se sono uscito dal ciclo perchè mi sono connesso o perchè ho finito la lista
    if (ptr == NULL)
    {
        fprintf(stderr, "Connessione non riuscita\n");
        exit(EXIT_FAILURE);
    }

    // SONO CONNESSO
    printf("mi sono connesso\n");
    fflush(stdout); // fflush vuol dire forza a svuotare il buffered in questo caso lo standard output

    // invio il parametro al server
    int nsend = write(sd, categoria, strlen(categoria));
    if (nsend < 0)
    {
        perror("err write client");
        fprintf(stderr, "Errore invio stringa");
        exit(EXIT_FAILURE);
    }

    // leggo la risposta dal server
    rxb_t rxb;
    rxb_init(&rxb, MAX_REQUEST_SIZE);
    char response[MAX_REQUEST_SIZE];
    size_t response_len;

    while (1)
    {
        /* Leggo la risposta del server e la stampo a video */

        /* Inizializzo il buffer response a zero e non uso l'ultimo
         * byte, così sono sicuro che il contenuto del buffer sarà
         * sempre null-terminated. In questo modo, posso interpretarlo
         * come una stringa C. Questa è un'operazione che va svolta
         * prima di leggere ogni nuova risposta. */
        memset(response, 0, sizeof(response));
        response_len = sizeof(response) - 1;

        /* Ricezione risultato */
        if (rxb_readline(&rxb, sd, response, &response_len) < 0)
        {
            /* Se sono qui, è perché ho letto un EOF. Significa che
             * il Server ha chiuso la connessione, per cui dealloco
             * rxb (opzionale) ed esco. */
            rxb_destroy(&rxb);
            fprintf(stderr, "Connessione chiusa dal server!\n");
            exit(EXIT_FAILURE);
        }

#ifdef USE_LIBUNISTRING
        /* Verifico che il testo sia UTF-8 valido */
        if (u8_check((uint8_t *)response, response_len) != NULL)
        {
            /* Server che malfunziona - inviata riga risposta
             * con stringa UTF-8 non valida */
            fprintf(stderr, "Response is not valid UTF-8!\n");
            close(sd);
            exit(EXIT_FAILURE);
        }
#endif

        /* Stampo riga letta da Server */
        puts(response);
    }

    close(sd);

    return 0;
}