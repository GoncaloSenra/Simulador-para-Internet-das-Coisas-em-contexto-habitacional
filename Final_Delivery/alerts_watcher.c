// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include "sys_header.h"


int alerts_watcher(int id) {

    write_logfile("PROCESS ALERTS_WATCHER CREATED\n");
  	if (DEBUG) printf("AW pid: %d\n", getpid());
  	
  	sem_wait(mutex_shm);
  	sh_var->alerts_watcher_pid = getpid();
  	sem_post(mutex_shm);
  	
	// Alerts_watcher awakes when cond_var is signaled		
	while (1) {
		pthread_mutex_lock(&sh_var->mutex_cond);
		
		while(1) {
			pthread_cond_wait(&sh_var->sens_watcher, &sh_var->mutex_cond);
			
			sem_wait(mutex_shm);
			if (sh_var->terminate == 1) {
				pthread_mutex_unlock(&sh_var->mutex_cond);
				write_logfile("PROCESS ALERTS_WATCHER TERMINATED\n");
				exit(0);
			}
			sem_post(mutex_shm);
			
			int i;
			int j;
			
			// Searches for a sensor which the last value is between the alert range
			sem_wait(mutex_shm);
			for (i = 0; i < sh_var->MAX_ALERTS; i++) {
				if (strcmp(sh_var->alerts[i].id, "") != 0) {
					for (j = 0; j < sh_var->MAX_KEYS; j++) {
						if (strcmp(sh_var->keys[j].id, sh_var->alerts[i].key) == 0) {
							if (sh_var->keys[j].last_value >= sh_var->alerts[i].min && sh_var->keys[j].last_value <= sh_var->alerts[i].max) {
								Message msg;
		    					
								msg.msgtype = (long) sh_var->alerts[i].id_user;
								sprintf(msg.buffer, "ALERT[%s]: %s = %d", sh_var->alerts[i].id, sh_var->alerts[i].key, sh_var->keys[j].last_value);
								puts(msg.buffer);
								msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0);
								
								char mes[512];
				   				sprintf(mes, "ALERTS_WATCHER: ALERT %s, %s %d to %d (FROM USER %d) TRIGGERED\n", sh_var->alerts[i].id, sh_var->alerts[i].key, sh_var->alerts[i].min, sh_var->alerts[i].max, sh_var->alerts[i].id_user);
				   				write_logfile(mes);
								

							}
							break;
						}
					}
				}
			}
			sem_post(mutex_shm);
		}
		
		pthread_mutex_unlock(&sh_var->mutex_cond);
	}


    return 0;
}
