#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> //Per il socket AF_INET

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024
void Gestore(int signo)
{
    printf("\nCtrl c. Termino\n");
    exit(0);
}
int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 4)
    {
        fprintf(stderr, "Errore hai inserito un numero diverso di argomenti\n");
        exit(1);
    }
    signal(SIGINT, Gestore);
    // strtol(argomento, ultimo argomento nel caso avessi una stringa composta da numeri e lettere, 10 perchè è un numero in base 10)
    int port = (int)strtol(argv[1], NULL, 10); // cast di un intero da long
    printf("%d", port);

    struct sockaddr_in serv_addr;
    char buffer[MAX_BUFFER_SIZE] = {0}; // non so perchè la devo mettere a 0
    char input[MAX_BUFFER_SIZE];
    int sock;
    // socket è come pipe ma tra server :)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) // socket(tipologia di rete(IPV4 oIPV6), Socket si riferisce ad una comunicazione,tipologia del protocollo precedente 0 = sceglie il SO)
    {                                                 // Esistono SOCK_STREAM per TCP e SOCK_DGRAM per udp
        perror("socket");
        exit(2);
    }
    serv_addr.sin_family = AF_INET;   // metto la tipologia dell'indirizzo sulla struct
    serv_addr.sin_port = htons(port); // converte il numero da byte di host a byte di rete
    // facendo cosi la mia porta da private (la vede solo la mia macchina) diventa public (la possono vedere gli altri)

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) // praticamente fa la stessa cosa di htons ma con l'indirizzo.
    {                                                          // e inserisco il valore dentro la struct
        perror("inet_pton");
        exit(2);
    }
    if (argc == 4)
    {
    }

    return 0;
}