// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h> 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/msg.h>

#define MSG_KEY 1234

typedef struct {
    long msgtype;
    char buffer[1024];
} Message;

int mqid;

//sem_t * mutex_pipe;

void sigint(){
	//sem_close(mutex_pipe);
	msgctl(mqid, IPC_RMID, 0);
	printf("\nUser_console terminating!\n");
	exit(0);
}

int main(int argc, char *argv[]) {
	
	signal(SIGINT, sigint);
	
    if (argc != 2) {
        printf("Error!\n");
        return -1;
    } else{
		int tam = atoi(argv[1]);
		if (tam > 0) {
        	printf("Welcome %s!\n", argv[1]);
    	} else {
    		printf("The ID must be a number greater than zero!\n");
    		return -1;
    	}
    }
    
    // Opens the pipe for writing
    int fd;
	if ((fd = open("CONSOLE_PIPE", O_WRONLY)) < 0) {
		perror("Cannot open pipe for writing: ");
		exit(0);
	}

	// Create the Message Queue
	if((mqid = msgget(MSG_KEY , IPC_CREAT|0777)) < 0) {
		perror("Error creating message queue!\n");
		exit(0);
	}
	
	printf("----> %d\n", mqid);

	char buffer[1024] = "";
	
	// Create Semaphore
    //sem_unlink("MUTEX_CONSOLE_PIPE");
	//mutex_pipe = sem_open("MUTEX_CONSOLE_PIPE", O_CREAT|O_EXCL, 0777, 1);

    // Print MENU
    printf("\nexit\nstats\nreset\nsensors\nadd_alert [id] [chave] [min] [max]\nremove_alert [id]\nlist_alerts\n\n> ");

    char option[100];
    while(fgets(option, 100, stdin)) {
    
    	Message msg;

        char* tokens [5];
 
        char* token;
        int count = 0;
        int notValid = 0;
        token = strtok(option, " \n");
        int errPipe = -1;

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
                sigint();
                return 0;
            }
        } else if (strcmp(tokens[0], "stats") == 0) {
            if (count != 1) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                sprintf(buffer, "%s#STATS", argv[1]);
                errPipe = write(fd, &buffer, 1024);
                
                msgrcv(mqid, &msg, sizeof(msg)-sizeof(long), (long) atoi(argv[1]), 0);
                //printf("RCV: %d\n", msg.buffer);
                puts(msg.buffer);
            }
        } else if (strcmp(tokens[0], "reset") == 0) {
            if (count != 1) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                sprintf(buffer, "%s#RESET", argv[1]);
                errPipe = write(fd, &buffer, 1024);
            }
        } else if (strcmp(tokens[0], "sensors") == 0) {
            if (count != 1) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                sprintf(buffer, "%s#SENSORS", argv[1]);
                errPipe = write(fd, &buffer, 1024);
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
                printf("KEY wrong size!\n");
                notValid = 1;
            }

            for (i = 0; i < strlen(tokens[2]); i++){
                if (!(isalnum(tokens[2][i]) || tokens[2][i] ==  '_')){
                    printf("KEY must be alphanumeric!\n");
                    notValid = 1;
                }
            }
            
            if (atoi(tokens[3])>atoi(tokens[4])){
            	printf("Max must be higher than Min\n");
            	notValid=1;
            }

            if (notValid == 0){
                sprintf(buffer, "%s#ADD_ALERT#%s#%s#%s#%s", argv[1], tokens[1], tokens[2], tokens[3], tokens[4]);
                errPipe = write(fd, &buffer, 1024);
                
            }
        } else if (strcmp(tokens[0], "remove_alert") == 0) {
            if (count != 2) {
                printf("Not valid!\n");
                notValid = 1;
            }
            if (notValid == 0) {
                printf("Not implemented!");
            }
        } else if (strcmp(tokens[0], "list_alerts") == 0) {
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
        
        //printf("status code : %d\n", errPipe);

		if (errPipe == -1) {
			printf("ERROR: pipe does not exist\n");
			sigint();
		}
        // Print MENU
        printf("\nexit\nstats\nreset\nsensors\nadd_alert [id] [chave] [min] [max]\nremove_alert [id]\nlist_alerts\n\n> ");
		memset(buffer, 0, 1024);
    }

}
