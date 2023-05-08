// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include "sys_header.h"


int worker(int id) {

    char* mes = malloc(sizeof(char) * (20));
    sprintf(mes, "WORKER %d READY\n", id);
    write_logfile(mes);
	
	close(channels[id-1][1]);

    sem_wait(mutex_shm);
    sh_var->workers[id-1].pid = getpid();
    sh_var->workers[id-1].active = 0;
	sh_var->teste++;
	//sh_var->N_WORKERS--;
	//sh_var->workers[id-1].active = 1;
    //printf("TESTE: %d\n", sh_var->teste);
    sem_post(mutex_shm);
    
    char buffer[1024];
	
    
    while (1) {
    	memset(buffer, 0, 1024);
    	//printf("ID %d\n", id-1);
		    	
    	/*int tam = */read(channels[id-1][0], &buffer, 1024);
    	//buffer[tam -1] = '\0';
    	
    	sem_wait(mutex_shm);
    	sh_var->workers[id-1].active = 1;
    	sem_post(mutex_shm);
    	
    	printf("->WORKER[%d]: %s\n", id, buffer);
		

 
        char* token;
        token = strtok(buffer, "#\n");
        
        puts(token);

        if (strcmp(token, "SENSOR") == 0)
        {
        	token = strtok(NULL, "#\n");
        	
        	pthread_mutex_lock(&sh_var->mutex_cond);

        	sem_wait(mutex_shm);
			int i;

			for (i = 0; i < sh_var->MAX_SENSORS; i++) {
				long tam = strlen(sh_var->sensors[i].id);

				if (tam == 0) {
					strcpy(sh_var->sensors[i].id, token);
					//sh_var->sensors[i].id = strdup(token);
					printf("NEW SENSOR\n");

					break;
				} else {
					if (strcmp(token, sh_var->sensors[i].id) == 0) {
						printf("OLD SENSOR\n");
						break;
					}
				}
			}
			
			token = strtok(NULL, "#\n");
			puts(token);
			int num = 0;
			int new = 0;
			for (i = 0; i < sh_var->MAX_KEYS; i++) {
				long tam = strlen(sh_var->keys[i].id);
				
				if (tam == 0) {
					strcpy(sh_var->keys[i].id, token);
					//sh_var->keys[i].id = strdup(token);
					
					printf("NEW KEY\n");
					new = 1;
					break;
				} else {
					if (strcmp(token, sh_var->keys[i].id) == 0) {
						printf("OLD KEY\n");
						break;
					}
				}
				num++;
			}
			
			int value = atoi(strtok(NULL, "#\n"));
			if (new == 1) {
				
				sh_var->keys[num].last_value = value;
				sh_var->keys[num].min = value;
				sh_var->keys[num].max = value;
				sh_var->keys[num].avg = value;
				sh_var->keys[num].count = 1;
			} else {
				sh_var->keys[num].count++;
				sh_var->keys[num].last_value = value;
				
				if (sh_var->keys[num].min > value) {
					sh_var->keys[num].min = value;
				}
				if (sh_var->keys[num].max < value) {
					sh_var->keys[num].max = value;
				}
				
				sh_var->keys[num].avg = ((sh_var->keys[num].avg * (sh_var->keys[num].count - 1)) + value) / sh_var->keys[num].count;
			}
			
			printf("STATS\nVALUE: %d\nMIN: %d\nMAX: %d\nAVG: %0.5f\nCOUNT: %d\n", sh_var->keys[num].last_value, sh_var->keys[num].min, sh_var->keys[num].max, sh_var->keys[num].avg , sh_var->keys[num].count);
			
			
			sem_post(mutex_shm);
			
			pthread_cond_signal(&sh_var->sens_watcher);
			printf("SIGNALLLLLLLLLLLLLLLLLL\n");
			pthread_mutex_unlock(&sh_var->mutex_cond);
        } else {

        	int id = atoi(token);
        	token = strtok(NULL, "#\n");
        	
        	if (strcmp(token, "STATS") == 0) {
        		char text[1024];
        		strcpy(text, "Key\t\t\tLast\tMin\tMax\tAvg\tCount\n");
        		sem_wait(mutex_shm);
        		int j;
   				
   				char temp[1024];
				//temp = (char *) malloc(1024);
   				
   				for (j = 0; j < sh_var->MAX_KEYS; j++) {
   					if (strcmp(sh_var->keys[j].id, "") == 0) {
   						break;
   					} else {
   						sprintf(temp, "%s\t\t%d\t%d\t%d\t%0.3f\t%d\n", sh_var->keys[j].id, sh_var->keys[j].last_value, sh_var->keys[j].min, sh_var->keys[j].max, sh_var->keys[j].avg, sh_var->keys[j].count);
   						//temp[strlen(temp) - 1] = '\0';
   						puts(temp);
   						strcat(text, temp);
   						memset(temp, 0, 1024);
   					}
   				}
   				
   				sem_post(mutex_shm);
   				
   				Message msg;
        		
        		msg.msgtype = (long) id;
        		strcpy(msg.buffer, text);
        		
        		msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
        		
        		
        		
        	} else if (strcmp(token, "ADD_ALERT") == 0) {
        		token = strtok(NULL, "#\n");
        	
        		int j;
        		int exists = 0;
        		
        		sem_wait(mutex_shm);
        		
        		for (j = 0; j < sh_var->MAX_ALERTS; j++) {
					if (strcmp(sh_var->alerts[j].id, "") == 0) {

						strcpy(sh_var->alerts[j].id, token);
						token = strtok(NULL, "#\n");
						int k;
						for (k = 0; k < sh_var->MAX_KEYS; k++) {
							if (strcmp(sh_var->keys[k].id, "") == 0) {
								exists = 2;
								break;
							} else if (strcmp(sh_var->keys[k].id, token) == 0) {
								strcpy(sh_var->alerts[j].key, token);
								break;
							}	
								
						}
						if (exists == 2) {
							strcpy(sh_var->alerts[j].id, "");
							break;
						}
						strcpy(sh_var->alerts[j].key, token);
						token = strtok(NULL, "#\n");
						sh_var->alerts[j].min = atoi(token);	
						token = strtok(NULL, "#\n");
						sh_var->alerts[j].max = atoi(token);
						break;
					} else if (strcmp(sh_var->alerts[j].id, token) == 0) {
						exists = 1;
						break;
					}			
        		}
        		
        		sem_post(mutex_shm);
        		
        		Message msg;
        		
        		msg.msgtype = (long) id;
        		
        		if (exists == 1) {
        			strcpy(msg.buffer, "ERROR: ALREADY EXISTS");
        			msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
        		} else if (exists == 2) {
        			strcpy(msg.buffer, "ERROR: KEY NOT FOUND");
        			msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
        		} else {
        			strcpy(msg.buffer, "OK");
        			msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
        		}
        		
        	} else if(strcmp(token, "LIST_ALERTS") == 0){
				char text[1024];
        		strcpy(text, "ID\t\tKEY\t\t\tMin\tMax\n");
        		sem_wait(mutex_shm);
        		int j;
   				
   				char temp[1024];
				//temp = (char *) malloc(1024);
   				
   				for (j = 0; j < sh_var->MAX_ALERTS; j++) {
   					if (strcmp(sh_var->alerts[j].id, "") != 0) {
   						sprintf(temp, "%s\t\t%s\t\t%d\t%d\n", sh_var->alerts[j].id, sh_var->alerts[j].key, sh_var->alerts[j].min, sh_var->alerts[j].max);
   						//temp[strlen(temp) - 1] = '\0';
   						//puts(temp);
   						strcat(text, temp);
   						memset(temp, 0, 1024);
   					}
   				}
   				
   				sem_post(mutex_shm);
   				Message msg;
        		
        		msg.msgtype = (long) id;
        		strcpy(msg.buffer, text);
        		
        		msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
        			        		
        	} else if (strcmp(token, "RESET") == 0) {
        		
        		sem_wait(mutex_shm);
        		int j;
        		for (j = 0; j < sh_var->MAX_SENSORS; j++) {
        			if (strcmp(sh_var->sensors[j].id, "") == 0) {
        				break;
        			} else {
        				strcpy(sh_var->sensors[j].id, "");
        			}
        		}
        		
        		for (j = 0; j < sh_var->MAX_KEYS; j++) {
        			if (strcmp(sh_var->keys[j].id, "") == 0) {
        				break;
        			} else {
        				strcpy(sh_var->keys[j].id, "");
        				sh_var->keys[j].last_value = 0;
        				sh_var->keys[j].min = 0;
        				sh_var->keys[j].max = 0;
        				sh_var->keys[j].avg = 0;
        				sh_var->keys[j].count = 0;
        			}
        		}
        		
        		sem_post(mutex_shm);
        		
        		Message msg;
        		
        		msg.msgtype = (long) id;
        		strcpy(msg.buffer, "OK");
        		
        		msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
        		
        	} else if(strcmp(token, "SENSORS") == 0) {
		    	char text[1024];
				strcpy(text, "ID\n");
				
				sem_wait(mutex_shm);
				int j;
				char temp[1024];
				//temp = (char *) malloc(1024);
				
				for (j = 0; j < sh_var->MAX_SENSORS; j++) {
					if (strcmp(sh_var->sensors[j].id, "") != 0) {
						sprintf(temp, "%s\n", sh_var->sensors[j].id);
						//temp[strlen(temp) - 1] = '\0';
						//puts(temp);
						strcat(text, temp);
						memset(temp, 0, 1024);
					}
				}
				
				sem_post(mutex_shm);
				Message msg;
				
				msg.msgtype = (long) id;
				strcpy(msg.buffer, text);
				
				msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
		    } else if (strcmp(token, "REMOVE_ALERT") == 0) {
        		token = strtok(NULL, "#\n");
        	
        		int j;
        		int exists = 0;
        		
        		sem_wait(mutex_shm);
        		
        		for (j = 0; j < sh_var->MAX_ALERTS; j++) {
					if (strcmp(sh_var->alerts[j].id, "") == 0) {
						exists = 2;
						break;
					} else if (strcmp(sh_var->alerts[j].id, token) == 0) {
						strcpy(sh_var->alerts[j].id, "");
						strcpy(sh_var->alerts[j].key, "");
						sh_var->alerts[j].id_user = 0;
						sh_var->alerts[j].min = 0;
						sh_var->alerts[j].max = 0;
						break;
					}			
        		}
        		
        		sem_post(mutex_shm);
        		
        		Message msg;
        		
        		msg.msgtype = (long) id;
        		
        		if (exists == 2) {
        			strcpy(msg.buffer, "ERROR: KEY NOT FOUND");
        			msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
        		} else {
        			strcpy(msg.buffer, "OK");
        			msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
        		}
        		
        	}
        }
		
    	sem_wait(mutex_shm);
    	sh_var->workers[id-1].active = 0;
    	sem_post(mutex_shm);
		
		sem_post(active_workers);
    
    }
    free(mes);
    free(buffer);
    return 0;
}

