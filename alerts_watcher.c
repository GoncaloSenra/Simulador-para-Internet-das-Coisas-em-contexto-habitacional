// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include "sys_header.h"


int alerts_watcher(int id) {

    write_logfile("PROCESS ALERTS_WATCHER CREATED\n");

	int i;
	sleep(2);
	
	printf("ALERTS_WATCHER: N_WORKERS %d\n", sh_var->N_WORKERS);
	
	for (i = 0; i < sh_var->N_WORKERS;i++){
		printf("ALERTS_WATCHER: WORKER[%d] = %d\n", sh_var->workers[i].id, sh_var->workers[i].active);
	}

    return 0;
}
