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

    if (argc != 2) {
        printf("Error!\n");
        return -1;
    } else{
        printf("Welcome %s!\n", argv[1]);
    }

    // Print MENU
    printf("\nexit\nstats\nreset\nsensors\nadd_alert [id] [chave] [min] [max]\nremove_alert [id]\nlist_alerts\n\n> ");

    char option[100];
    while(fgets(option, 100, stdin)) {

        char* tokens [5];
 
        char* token;
        int count = 0;
        int notValid = 0;
        token = strtok(option, " \n");

        while (token != NULL) {
            tokens[count] = strdup(token);
            
            token = strtok(NULL, " \n");
            count++;
        }
        
        if (strcmp(tokens[0], "exit") == 0) {
            if (count != 1) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                return 0;
            }
        } else if (strcmp(tokens[0], "stats") == 0) {
            if (count != 1) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                printf("Not implemented!");
            }
        } else if (strcmp(tokens[0], "reset") == 0) {
            if (count != 1) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                printf("Not implemented!");
            }
        } else if (strcmp(tokens[0], "sensors") == 0) {
            if (count != 1) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                printf("Not implemented!");
            }
        } else if (strcmp(tokens[0], "add_alert") == 0) {
            if (count != 5) {
                printf("Not valid!\n");
                notValid = 1;
            }

            if (strlen(tokens[1]) < 3 || strlen(tokens[1]) > 32) {
                printf("ID wrong size!\n");
                notValid = 1;
            }
            int i;
            for (i = 0; i < strlen(tokens[1]); i++){
                if (!(isalnum(tokens[1][i]) || tokens[1][i] ==  '_')){
                    printf("ID must be alphanumeric!\n");
                    notValid = 1;
                }
            }

            if (strlen(tokens[2]) < 3 || strlen(tokens[2]) > 32) {
                printf("CHAVE wrong size!\n");
                notValid = 1;
            }

            for (i = 0; i < strlen(tokens[2]); i++){
                if (!(isalnum(tokens[2][i]) || tokens[2][i] ==  '_')){
                    printf("CHAVE must be alphanumeric!\n");
                    notValid = 1;
                }
            }

            if (notValid == 0){
                printf("Not implemented!");
                
            }
        } else if (strcmp(tokens[0], "remove_alert") == 0) {
            if (count != 2) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                printf("Not implemented!");
            }
        } else if (strcmp(tokens[0], "list_alert") == 0) {
            if (count != 1) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                printf("Not implemented!");
            }
        } else {
            printf("Not valid!\n");
        }

        // Print MENU
        printf("\nexit\nstats\nreset\nsensors\nadd_alert [id] [chave] [min] [max]\nremove_alert [id]\nlist_alerts\n\n> ");

    }

}