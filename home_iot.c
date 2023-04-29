// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include "sys_header.h"

#define MSG_KEY 1234

static FILE * log = NULL;

pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;

void exit_home_iot(){

	write_logfile("HOME_IOT SIMULATOR WAITING FOR LAST TASKS TO FINISH\n");
	
	// Join Threads
	sem_wait(mutex_shm);
	
	sh_var->terminate=1;
	
    pthread_cancel(sh_var->console_reader_t);
    unlink("CONSOLE_PIPE");
	write_logfile("THREAD CONSOLE_READER EXITING\n");
    
    pthread_cancel(sh_var->sensor_reader_t);
    unlink("SENSOR_PIPE");
	write_logfile("THREAD SENSOR_READER EXITING\n");
			
    pthread_cancel(sh_var->dispatcher_t);
    write_logfile("THREAD DISPATCHER EXITING\n");
    
	sem_post(mutex_shm);
	
	write_logfile("HOME_IOT SIMULATOR CLOSING\n");
	
    // Remove resources
    sem_close(mutex_shm);
	sem_unlink("MUTEX_SHM");
    
    sem_close(mutex_log);
	sem_unlink("MUTEX_LOG");
	
	sem_close(sem_qsize);
	sem_unlink("SEM_QSIZE");
	
	sem_close(sem_qcons);
	sem_unlink("SEM_QCONS");
	
	pthread_mutex_destroy(&mutex_queue);
	
	int i;
	for (i = 0; i < sh_var->N_WORKERS; i++) {
		kill(sh_var->workers[i].pid, SIGKILL);
	}
	
	shmdt(sh_var->workers);
	shmctl(shwid, IPC_RMID, NULL);
	shmdt(sh_var->sensors);
	shmctl(shsid, IPC_RMID, NULL);
	shmdt(sh_var->keys);
	shmctl(shkid, IPC_RMID, NULL);
	shmdt(sh_var->alerts);
	shmctl(shaid, IPC_RMID, NULL);
	shmdt(sh_var);
	shmctl(shmid, IPC_RMID, NULL);
	
	msgctl(mqid, IPC_RMID, 0);
	
	free(internalQ);

}

void sigint(int signum) { // handling of CTRL-C
	//printf("hellooooo\n");
	write_logfile("SIGNAL SIGINT RECEIVED\n");
	exit_home_iot();
	exit(0);
}

void write_logfile(char * message){

    time_t rawtime;
    struct tm * timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char* time_aux = malloc(sizeof(char) * (20 + strlen(message)));
    sprintf(time_aux, "%d:%d:%d ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    sem_wait(mutex_log);
    
    log = fopen("log.txt", "a");
    
    fwrite(time_aux, 1, strlen(time_aux), log);
    fwrite(message, 1, strlen(message), log);
    printf("%s", time_aux);
    printf("%s", message);
    
    fclose(log);
    
    sem_post(mutex_log);

    free(time_aux);

}

void * console_reader(void * id_t) {

    write_logfile("THREAD CONSOLE_READER CREATED\n");
    
	char buffer[1024] = "";
	
	while(1){
		
		long tam;
		
		// Opens the pipe for reading
		int fd;
		if ((fd = open("CONSOLE_PIPE", O_RDONLY)) < 0) {
			perror("Cannot open pipe for reading: ");
			exit(0);
		}
		
		do {
			tam = read(fd, buffer, 1024);
			//printf("tam: %ld\n", tam);
			buffer[tam-1] = '\0';
			
			if (strcmp(buffer, "") != 0){
				printf("CONSOLE_PIPE: %s\n", buffer);
				
				Node * newNode = (Node*) malloc(sizeof(Node*));
				newNode->data = (char*) malloc(1024);
				strcpy(newNode->data, buffer);
				newNode->next = NULL;
				newNode->prev = NULL;
				
				pthread_mutex_lock(&mutex_queue);
				
				if (internalQ->count == sh_var->QUEUE_SZ) {
					printf("FILA CHEIA!\n");
				}
				
				sem_wait(sem_qsize);
				
				printf("ENTROU NA FILA\n");
	
				if (internalQ->count == 0){
					internalQ->head = newNode;
					internalQ->tail = newNode;
					internalQ->count++;
				} else {
					internalQ->head->next = newNode;
					newNode->prev = internalQ->head;
					internalQ->head = newNode;
					internalQ->count++;
				}
				
				sem_post(sem_qcons);
				
				pthread_mutex_unlock(&mutex_queue);
				memset(buffer, 0, 1024);
				
			}
		} while(tam > 0);
		
	}

    pthread_exit(NULL);
}

void * sensor_reader(void * id_t) {

    write_logfile("THREAD SENSOR_READER CREATED\n");

	char buffer[1024] = "";
	
	while(1){
		
		long tam; 
		
		// Opens the pipe for reading
		int fd;
		if ((fd = open("SENSOR_PIPE", O_RDONLY)) < 0) {
			perror("Cannot open pipe for reading: ");
			exit(0);
		}
		
		do {
			tam = read(fd, buffer, 1024);
			buffer[tam-1] = '\0';
			
			if (strcmp(buffer, "") != 0){
				printf("SENSOR_PIPE: %s\n", buffer);
				
				Node * newNode = (Node*) malloc(sizeof(Node*));
				newNode->data = (char*) malloc(1024);
				
				sprintf(newNode->data, "SENSOR#%s", buffer);
				newNode->next = NULL;
				newNode->prev = NULL;
				
				pthread_mutex_lock(&mutex_queue);
				
				int value;
				sem_getvalue(sem_qsize, &value);
				if (value != 0)
				{
					//printf("entrou semaforo\n");
					sem_wait(sem_qsize);
				}
				//printf("VALUE >>>>>>> %d\n", value);
				if (internalQ->count == sh_var->QUEUE_SZ) {
					printf("FILA CHEIA!\n");
				} else {
					printf("ENTROU NA FILA\n");
	
					if (internalQ->count == 0){
						internalQ->head = newNode;
						internalQ->tail = newNode;
						internalQ->count++;
					} else {
						newNode->next = internalQ->tail;
						internalQ->tail->prev = newNode;
						internalQ->tail = newNode;
						internalQ->count++;
					}
					sem_post(sem_qcons);
				}
				
				pthread_mutex_unlock(&mutex_queue);
				//free(aux);
				memset(buffer, 0, 1024);
			}
		} while (tam > 0);
	}
	
    pthread_exit(NULL);
}

void * dispatcher(void * id_t) {

    write_logfile("THREAD DISPATCHER CREATED\n");
	
	int i;
	for (i = 0; i < sh_var->N_WORKERS; i++) {
		close(channels[i][0]);
	}
		
	while(1){
		/*
		//sem_wait(mutex_shm);
		if (sh_var->terminate == 1)
		{
			write_logfile("THREAD DISPATCHER EXITING\n");
			pthread_exit(NULL);
		}
		//sem_post(mutex_shm);
		*/
		
		//int value;
		//sem_getvalue(sem_qcons, &value);
		//printf(">> %d\n", value);
		sem_wait(sem_qcons);
		
		pthread_mutex_lock(&mutex_queue);
		
		Node * no = internalQ->head;
		if (internalQ->count == 1) {
			internalQ->head = NULL;
			internalQ->tail = NULL;
		} else {
			internalQ->head = internalQ->head->prev;
			internalQ->head->next = NULL;
		}
		
		internalQ->count--;
		
		pthread_mutex_unlock(&mutex_queue);
		
		sem_post(sem_qsize);
		
		printf("[DISPATCHER]: CONSUMED MESSAGE %s\n", no->data);
		
		sem_wait(mutex_shm);
		int k;
		for (k = 0; k < sh_var->N_WORKERS; k++) {
			if (sh_var->workers[k].active == 0) {
				printf("====%d\n", k);
				write(channels[k][1], no->data, 1024);
				break;
			}
		}
		sem_post(mutex_shm);
		
		free(no);
		
		
	}

}


int create_procs_threads() {

    // Create Workers
    int i;
    for (i = 0; i < sh_var->N_WORKERS; i++) {
        if (fork() == 0) {
        	signal(SIGINT, SIG_IGN);
            worker(i + 1);
            exit(0);
        }
    }

    // Create Alerts_Watcher
    if (fork() == 0) {
    	signal(SIGINT, SIG_IGN);
        alerts_watcher(0);
        exit(0);
    }
    
	
	//printf("teste\n");
	
	sem_wait(mutex_shm);
    // Create Console Reader thread
    int cr_id = 1;
    pthread_create(&sh_var->console_reader_t, NULL, console_reader, &cr_id);

    // Create Sensor Reader thread
    int sr_id = 2;
    pthread_create(&sh_var->sensor_reader_t, NULL, sensor_reader, &sr_id);

    // Create Dispatcher thread
    int d_id = 3;
    pthread_create(&sh_var->dispatcher_t, NULL, dispatcher, &d_id);
    
    sem_post(mutex_shm);
    
	pthread_join(sh_var->console_reader_t, NULL);
    pthread_join(sh_var->sensor_reader_t, NULL);
    pthread_join(sh_var->dispatcher_t, NULL);
    
    /*
    for (i = 0; i < sh_var->N_WORKERS+1; i++) {
        wait(NULL);
    }
    */
    
    
    return 0;
}

int main(int argc, char *argv[]) {
	
	//terminates when CTRL-C is pressed
	signal(SIGINT,sigint);

    log = fopen("log.txt", "w");
    fclose(log);

    int failure = 0;

    if (argc != 2) {
        printf("Wrong argc! %d\n", argc);
        return -1;
    }

    // Create Shared Memory
    shmid = shmget(IPC_PRIVATE, sizeof(Shared_var *), IPC_CREAT|0777);

    // Attach shared memory
	sh_var = (Shared_var *) shmat(shmid, NULL, 0);


    FILE * config = fopen(argv[1], "r");
    char line[30];

    fgets(line, 30, config);
    int QUEUE_SZ = atoi(line);

    if (QUEUE_SZ < 1) {
        printf("QUEUE_SZ must be (>=1)\n");
        failure = 1;
    }

    sh_var->QUEUE_SZ = QUEUE_SZ;

    fgets(line, 30, config);
    int N_WORKERS = atoi(line);

    if (N_WORKERS < 1) {
        printf("N_WORKERS must be (>=1)\n");
        failure = 1;
    }
    
    sh_var->N_WORKERS = N_WORKERS;
    
    // Create shared memory for workers info
    shwid = shmget(IPC_PRIVATE, sizeof(Worker) * N_WORKERS, IPC_CREAT|0777);
    sh_var->workers = (Worker*) shmat(shwid, NULL, 0);
    
    int j;
    for (j = 0; j < N_WORKERS; j++) {
    	sh_var->workers[j].id = j+1; 
    	sh_var->workers[j].active = 0;
    }

    fgets(line, 30, config);
    int MAX_KEYS = atoi(line);

    if (MAX_KEYS < 1) {
        printf("MAX_KEYS must be (>=1)\n");
        failure = 1;
    }

    sh_var->MAX_KEYS = MAX_KEYS;
	
	// Create shared memory for keys info
    shkid = shmget(IPC_PRIVATE, sizeof(Key) * MAX_KEYS, IPC_CREAT|0777);
    sh_var->keys = (Key*) shmat(shkid, NULL, 0);

    
    for (j = 0; j < MAX_KEYS; j++) {
    	sh_var->keys[j].id = ""; 
    	//continue;
    }
    
    
	
    fgets(line, 30, config);
    int MAX_SENSORS = atoi(line);

    if (MAX_SENSORS < 1) {
        printf("MAX_SENSORS must be (>=1)\n");
        failure = 1;
    }

    sh_var->MAX_SENSORS = MAX_SENSORS;
    
    // Create shared memory for sensors info
    shsid = shmget(IPC_PRIVATE, sizeof(Sensor) * MAX_SENSORS, IPC_CREAT|0777);
    sh_var->sensors = (Sensor*) shmat(shsid, NULL, 0);
    
    for (j = 0; j < MAX_SENSORS; j++) {
    	sh_var->sensors[j].id = ""; 
    }

    fgets(line, 30, config);
    int MAX_ALERTS = atoi(line);

    if (MAX_ALERTS < 0) {
        printf("MAX_ALERTS must be (>=0)\n");
        failure = 1;
    }

    sh_var->MAX_ALERTS = MAX_ALERTS;
    
    // Create shared memory for alerts info
    shaid = shmget(IPC_PRIVATE, sizeof(Alert) * MAX_ALERTS, IPC_CREAT|0777);
    sh_var->alerts = (Alert*) shmat(shaid, NULL, 0);

	 for (j = 0; j < MAX_ALERTS; j++) {
    	sh_var->alerts[j].id = ""; 
    }

    fclose(config);

    sh_var->teste = 0;

    // Create Semaphores
    sem_unlink("MUTEX_SHM");
	mutex_shm = sem_open("MUTEX_SHM", O_CREAT|O_EXCL, 0777, 1);
    sem_unlink("MUTEX_LOG");
	mutex_log = sem_open("MUTEX_LOG", O_CREAT|O_EXCL, 0777, 1);
	sem_unlink("SEM_QSIZE");
	sem_qsize = sem_open("SEM_QSIZE", O_CREAT|O_EXCL, 0777, sh_var->QUEUE_SZ);
	sem_unlink("SEM_QCONS");
	sem_qcons = sem_open("SEM_QCONS", O_CREAT|O_EXCL, 0777, 0);
	
	
	// Creates the named pipe if it doesn't exist yet
	if ((mkfifo("CONSOLE_PIPE", O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)) {
		perror("Cannot create pipe: ");
		exit(0);
	}

	if ((mkfifo("SENSOR_PIPE", O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)) {
		perror("Cannot create pipe: ");
		exit(0);
	}
	
	// Creates the (N_WORKERS) unamed pipes
	
	channels = (int**)malloc(N_WORKERS * sizeof(int*));
	
	int k;
	for (k = 0; k < N_WORKERS; k++) {
		channels[k] = (int*)malloc(2 * sizeof(int));
	}
	
	for (k = 0; k < N_WORKERS; k++) {
		pipe(channels[k]);
	}
	
	
	// Create the INTERNAL_QUEUE
	internalQ = (Queue*) malloc(sizeof(Queue*));
	internalQ->head = NULL;
	internalQ->tail = NULL;
	internalQ->count = 0;
	
	// Create the Message Queue
	if((mqid = msgget(MSG_KEY , IPC_CREAT|0777)) < 0) {
		perror("Error creating message queue!\n");
		exit(0);
	}
	
	//printf("----> %d\n", mqid);

    write_logfile("HOME_IOT SIMULATOR STARTING\n");
    
    // Function to create Processes and Threads
    if (failure == 0)
        create_procs_threads();
    else 
    	exit_home_iot();
    	
    //wait(NULL);

    return 0;
}
