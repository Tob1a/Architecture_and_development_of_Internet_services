/*
    dopo aver compilato gcc client
    gcc -c server.c
    gcc rxb.o utils.o server.o -o SERVER
*/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //per la memset
#include <netdb.h>  //serve per la socket e per la struct
#include <signal.h>
#include <unistd.h>

// Librerie prof
#include "rxb.h"
#define MAX_REQUEST_SIZE 4096 // limite di trasmissione TCP
#include "utils.h"

/*void sigchld_handler(int signo)
{
    int status;

    (void)signo; // ignora la variabile di tipo signo

    while (waitpid(-1, &status, WNOHANG) > 0) // attende che il processo figlio (con pid -1 in questo caso) termini in questo caso siccome ho usato WNOHANG allora non sospende l'esecuzione se lo stato non è immediatamente disponibile
        // nel caso gli passo -1 come pid allora indico lo stato di tutti i processi figli
        // status e' la variabile che contiene lo stato del processo
        // waitpid restituisce un valore uguale al pid processo del figlio. Se ho -1 ho un errore. Nel caso avessi 0 e' nel caso tutti i miei processi non sono immediatamente disponibili
        continue;
}
*/
int main(int argc, char *argv[])
{
    int err, sd;
    struct sigaction sa; // La struttura sigaction serve per gestire un determinato flag di un segnale
                         /*
                     
                         struct sigaction {
                             void     (*sa_handler)(int);       // Puntatore al gestore del segnale
                             void     (*sa_sigaction)(int, siginfo_t *, void *); // Per gestori avanzati
                             sigset_t   sa_mask;                // Maschera dei segnali bloccati durante l'esecuzione
                             int        sa_flags;               // Opzioni di comportamento
                         };
                     
                         */
    struct addrinfo hints, *res;

    if (argc != 2)
    {
        fprintf(stderr, "Errore non hai inserito la porta");
        exit(EXIT_FAILURE);
    }

    signal(SIGPIPE, SIG_IGN); // nel caso la pipe dovesse mandare un segnale lo ignoro

    sigemptyset(&sa.sa_mask); // setta a 0 la maschera
    sa.sa_flags = SA_RESTART; // SA_RESTART è un flag che serve per dire che quando alcune chiamate di sistema(es. read, write,..) vengono interrotte da un segnale di farle ripartire automaticamente
    // sa.sa_handler = sigchld_handler;

    // err = sigaction(SIGCHLD, &sa, NULL); // ok ora identifica il segnale (SIGCHLD) da gestire, punta a una struttura (sa) che gli dice come gestire il segnale e al terzo posto posso mettere una struttura dove viene messa l'attuale gestione del segnale
    //  sigaction ritorna 0 in SUCCESS oppure -1 in caso FAIL
    //  SIGCHLD è un segnale inviato a un processo genitore quando:
    //  1 Un processo figlio termina.
    //  2 Un processo figlio è stato sospeso (o ripreso).
    //  3 Un processo figlio cambia stato (ad esempio, termina in modo anomalo).
    /*if (err < 0)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }*/

    // Inizializzazione di hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // aggiunge questa parte al posto del client

    // apro una socket
    err = getaddrinfo(NULL, argv[1], &hints, &res); // NULL va bene per indicare l'indirizzo corrente
    if (err != 0)
    {
        fprintf(stderr, "Errore getaddrinfo\n");
        exit(EXIT_FAILURE);
    }
    // a differenza del client non devo scorrerle tutte per connettermi ma devo solo crearla
    sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int on = 1;
    err = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); // mi setta la socket
    // SOL_SOCKET indica che l'opzione viene effettuata a livello socket (anzichè ad un protocollo specifico)
    // SO_REUSEADDR indica alla socket che posso riutilizzare lo stesso indirizzo e la stessa porta
    // on e' il flag (o 0 o 1) per dirgli quando l'opzione REUSE deve essere abilitata
    // restituisce
    if (err < 0)
    {
        perror("Errore setsockopt");
        exit(EXIT_FAILURE);
    }

    err = bind(sd, res->ai_addr, res->ai_addrlen); // fa le veci della connect per il server
    if (err < 0)
    {
        perror("Errore bind");
        exit(EXIT_FAILURE);
    }

    err = listen(sd, SOMAXCONN); // il server ascolta i segnali
    // sd è dove li vuole sentire e il secondo parametro indica il numero massimo di connessioni pendenti in quel socket
    // SOMAXCONN è il valore massimo possibile (128)
    if (err < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res); // ok la creazione è andata a buon fine. Posso dealloc la res

    while (1)
    {
        int ns, p0;
        ns = accept(sd, NULL, NULL); // accetta una connessione in entrata
        // primo parametro indica la socket che è in ascolto.
        // secondo parametro indica se si vuole che vengano restituiti l'ip e la porta del client
        // terzo parametro indica se viene restituito la dimensione dell'indirizzo del client
        // ns è il puntatore ad un nuovo file descriptor per il socket dedicato alla connessione accettata
        // se ns = -1 errore
        if (ns < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        p0 = fork();
        if (p0 < 0)
        {
            perror("pid");
            exit(EXIT_FAILURE);
        }

        if (p0 == 0)
        {
            rxb_t rxb; // inizio a creare il buffer per la risposta
            rxb_init(&rxb, MAX_REQUEST_SIZE);

            // signal(SIGCHLD, SIG_DFL); // dice di non fare niente ai processi figli che terminano in modo che diventino zombie (ci pensera il waitpid)

            while (1)
            {
                int p1p2[2];
                int p1, p2, status;
                char mese[1024];
                char categoria[1024];
                char numero[1024];
                size_t mese_len, categoria_len, numero_len;
                char *end_request = "--- END REQUEST ---\n";

                memset(mese, 0, sizeof(mese)); // mette tutti i bit di mese a 0
                mese_len = sizeof(mese) - 1;

                err = rxb_readline(&rxb, ns, mese, &mese_len); // inizio a leggere il buffer e lo metto in mese
                if (err < 0)
                {
                    fprintf(stderr, "Errore readline mese\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

                /* Ricorda di cambiare mese con la variabile */
#ifdef HAVE_LIBUNISTRING
                if (u8_check((uint8_t *)mese, mese_len) != NULL)
                {
                    fprintf(stderr, "Testo UTF-8 non valido");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                memset(categoria, 0, sizeof(categoria));
                categoria_len = sizeof(categoria) - 1;

                err = rxb_readline(&rxb, ns, categoria, &categoria_len);
                if (err < 0)
                {
                    fprintf(stderr, "Errore readline categoria\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef HAVE_LIBUNISTRING
                if (u8_check((uint8_t *)categoria, categoria_len) != NULL)
                {
                    fprintf(stderr, "Testo UTF-8 non valido");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                memset(numero, 0, sizeof(numero));
                numero_len = sizeof(numero) - 1;

                err = rxb_readline(&rxb, ns, numero, &numero_len);
                if (err < 0)
                {
                    fprintf(stderr, "Errore readline numero\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef HAVE_LIBUNISTRING
                if (u8_check((uint8_t *)numero, numero_len) != NULL)
                {
                    fprintf(stderr, "Testo UTF-8 non valido");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // ok ora inizio a creare una pipe
                if (pipe(p1p2) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                p1 = fork();
                if (p1 < 0)
                {
                    perror("p1");
                    exit(EXIT_FAILURE);
                }
                if (p1 == 0)
                {
                    char percorso[1024];
                    snprintf(percorso, sizeof(percorso), "test/%s/%s.txt", mese, categoria);

                    close(1);
                    dup(p1p2[1]);
                    close(p1p2[1]);

                    close(p1p2[0]);
                    close(ns);
                    execlp("sort", "sort", "-r", "-n", percorso, NULL);
                    perror("sort");
                    exit(EXIT_FAILURE);
                }

                close(p1p2[1]);
                p2 = fork();
                if (p2 < 0)
                {
                    perror("p2");
                    exit(EXIT_FAILURE);
                }
                if (p2 == 0)
                {
                    close(0);
                    dup(p1p2[0]);
                    close(1);
                    dup(ns);

                    close(p1p2[0]);
                    close(ns);
                    execlp("head", "head", "-n", numero, NULL);
                    perror("head");
                    exit(EXIT_FAILURE);
                }
                close(p1p2[0]);

                // attendo terminazione
                waitpid(p1, &status, 0);
                waitpid(p2, &status, 0);

                // Mando la stringa di terminazione
                err = write_all(ns, end_request, strlen(end_request));
                if (err < 0)
                {
                    perror("write_all");
                    exit(EXIT_FAILURE);
                }
            }
            exit(0);
        }
        close(ns);
    }
    close(sd);
    return 0;
}