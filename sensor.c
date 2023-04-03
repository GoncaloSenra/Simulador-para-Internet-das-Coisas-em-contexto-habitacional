//compile with: gcc -Wall -pthread sharedvariable_posix.c -o svar
//using POSIX named semaphores

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h> // include POSIX semaphores

/*insert here the other libraries needed*/
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<sys/types.h>
#include <string.h>
#include <ctype.h>


int main(int argc, char *argv[]) {
    int i;

    if (argc != 6) {
        printf("%d\n", argc);
        printf("Error!\n");
        printf("sensor {identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {valor inteiro mínimo a ser enviado} {valor inteiro máximo a ser enviado}\n");
        return -1;
    }

    if (strlen(argv[1]) < 3 || strlen(argv[1]) > 32) {
        printf("ID wrong size!\n");
    }

    for (i = 0; i < strlen(argv[1]); i++){
        if (!(isalnum(argv[1][i]) || argv[1][i] == '_')){
            printf("ID must be alphanumeric!\n");
            return -1;
        }
    }

    if (strlen(argv[3]) < 3 || strlen(argv[3]) > 32) {
        printf("CHAVE wrong size!\n");
    }

    for (i = 0; i < strlen(argv[3]); i++){
        if (!(isalnum(argv[3][i]) || argv[3][i] ==  '_')){
            printf("CHAVE must be alphanumeric!\n");
            return -1;
        }
    }

    // Verifiar se intervalo de segundos é um número interiro
    for (i = 0; i < strlen(argv[2]); i++){
        if (!isdigit(argv[2][i])){
            printf("Intervalo de segundos must be a number!\n");
            return -1;
        }
    }

    // Verifiar se intervalo de segundos é >= 0
    int sec = atoi(argv[2]);
    if (sec < 0) {
        printf("Intervalo de segundos must be a number!\n");
    }

    // Verifiar se Min é um número interiro
    for (i = 0; i < strlen(argv[4]); i++){
        if (!isdigit(argv[4][i])){
            printf("Min must be a number!\n");
            return -1;
        }
    }

    // Verifiar se Max é um número interiro
    for (i = 0; i < strlen(argv[5]); i++){
        if (!isdigit(argv[5][i])){
            printf("Max must be a number!\n");
            return -1;
        }
    }

    int min = atoi(argv[4]);
    int max = atoi(argv[5]);

    if (min > max) {
        printf("Max must be higher then min\n");
    }




}