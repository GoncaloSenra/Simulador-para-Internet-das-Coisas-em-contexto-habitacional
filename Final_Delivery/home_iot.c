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
	write_logfile("THREAD CONSOLE_READER TERMINATED\n");
    
    pthread_cancel(sh_var->sensor_reader_t);
	write_logfile("THREAD SENSOR_READER TERMINATED\n");
			
    pthread_cancel(sh_var->dispatcher_t);
    write_logfile("THREAD DISPATCHER TERMINATED\n");
    
	sem_post(mutex_shm);
	
	write_logfile("HOME_IOT SIMULATOR CLOSING\n");
	
	int k;
	for (k = 0; k < sh_var->N_WORKERS; k++) {
		wait(&sh_var->workers[k].pid);
		close(channels[k][0]);
		close(channels[k][1]);
		free(channels[k]);
	}
	
	free(channels);
	
	pthread_mutex_lock(&sh_var->mutex_cond);
	pthread_cond_signal(&sh_var->sens_watcher);
	pthread_mutex_unlock(&sh_var->mutex_cond);
	
	wait(&sh_var->alerts_watcher_pid);
	
	// PRINT INTERNALQ SLOTS
	int i;
	char mes [1024];
	Node * temp = internalQ->head;
	
	strcpy(mes, "INTERNALQ:\n");
	
	if (internalQ->count == 0) {
		char aux [50];
		
		sprintf(aux, "\tEMPTY\n");
		strcat(mes, aux);
	} else {
		
		for (i = 0; i < internalQ->count; i++) {
			char aux [50];
			
			sprintf(aux, "\t%d: %s\n", i, temp->data);
			strcat(mes, aux);

			memset(aux, 0, 50);
			temp = temp->prev;
		}
		
	}
	
	write_logfile(mes);
	
	// Remove resources
    sem_close(mutex_shm);
	sem_unlink("MUTEX_SHM");
    
    //sem_close(mutex_log);
	//sem_unlink("MUTEX_LOG");
	
	sem_close(sem_qsize);
	sem_unlink("SEM_QSIZE");
	
	sem_close(sem_qcons);
	sem_unlink("SEM_QCONS");
	
	sem_close(active_workers);
	sem_unlink("A_WORKERS");
	
	unlink("CONSOLE_PIPE");
	unlink("SENSOR_PIPE");

	
	pthread_mutex_destroy(&mutex_queue);
	pthread_mutex_destroy(&sh_var->mutex_cond);
	pthread_cond_destroy(&sh_var->sens_watcher);

	
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

void sigint(int signum) {
	write_logfile("SIGNAL SIGINT RECEIVED\n");
	exit_home_iot();
	exit(signum);
}

void signals_to_ignore(int signum) {
	char temp[50];
	sprintf(temp, "SIGNAL %d RECEIVED\n", signum);
	write_logfile(temp);
	
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
    
    fflush(stdout);
    
    fclose(log);
    
    sem_post(mutex_log);

    free(time_aux);

}

void * console_reader(void * id_t) {

    write_logfile("THREAD CONSOLE_READER CREATED\n");
    
	char buffer[1024] = "";
	
	// If the writer closes the pipe the console reader reopens it
	while(1){
		
		long tam;
		
		// Opens the pipe for reading
		int fd;
		if ((fd = open("CONSOLE_PIPE", O_RDONLY)) < 0) {
			write_logfile("CANNOT OPEN CONSOLE_PIPE FOR READING\n");
			sigint(1);
		}
		
		do {
			tam = read(fd, buffer, 1024);
			buffer[tam-1] = '\0';
			
			if (strcmp(buffer, "") != 0){
				if (DEBUG) printf("CONSOLE_PIPE: %s\n", buffer);
				
				Node * newNode = (Node*) malloc(sizeof(Node));
				newNode->data = (char*) malloc(1024);
				strcpy(newNode->data, buffer);
				newNode->isSensor = 0;
				newNode->next = NULL;
				newNode->prev = NULL;
				
				pthread_mutex_lock(&mutex_queue);
				
				if (internalQ->count == sh_var->QUEUE_SZ) {
					if (DEBUG) printf("FILA CHEIA!\n");
				}
				
				// If the queue is full the task sent by the user won't be lost the thread should wait for an empty slot
				sem_wait(sem_qsize);
				
				if (DEBUG) printf("ENTROU NA FILA\n");
				
				// If the queue is empty
				if (internalQ->count == 0){
					internalQ->head = newNode;
					internalQ->tail = newNode;
					internalQ->count++;
				} else {
					
					// If the queue is not empty this cycle will search for the last user_console node and the new node will be placed at the end of tht node
					Node * temp = internalQ->head;
					while (temp != NULL && temp->isSensor == 0) {
						temp = temp->prev;
					}
					if (temp == NULL) {
						newNode->next = internalQ->tail;
						internalQ->tail->prev = newNode;
						internalQ->tail = newNode;
					} else if (temp->next == NULL) {
						temp->next = newNode;
						newNode->prev = temp;
						internalQ->head = newNode;
					} else {
						newNode->prev = temp;
						newNode->next = temp->next;
						temp->next->prev = newNode;
						temp->next = newNode;
					}
					
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
	
	// If the writer closes the pipe the sensor reader reopens it
	while(1){
		long tam; 
		
		// Opens the pipe for reading
		int fd;
		if ((fd = open("SENSOR_PIPE", O_RDONLY)) < 0) {
			write_logfile("CANNOT OPEN CONSOLE_PIPE FOR READING\n");
			sigint(1);
		}
		
		do {
			tam = read(fd, buffer, 1024);
			buffer[tam-1] = '\0';
			
			if (strcmp(buffer, "") != 0){
				if (DEBUG) printf("SENSOR_PIPE: %s\n", buffer);
				
				Node * newNode = (Node*) malloc(sizeof(Node));
				newNode->data = (char*) malloc(1024);
				
				sprintf(newNode->data, "SENSOR#%s", buffer);
				newNode->isSensor = 1;
				newNode->next = NULL;
				newNode->prev = NULL;
				
				pthread_mutex_lock(&mutex_queue);
				
				// If the queue is full this thread won´t wait for an empty slot, instead the message will be discarded
				int value;
				sem_getvalue(sem_qsize, &value);
				if (value != 0)
				{
					sem_wait(sem_qsize);
				}

				if (internalQ->count == sh_var->QUEUE_SZ) {
					if (DEBUG) printf("FILA CHEIA!\n");
				} else {
					if (DEBUG) printf("ENTROU NA FILA\n");
	
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
		
		sem_wait(sem_qcons);
		
		pthread_mutex_lock(&mutex_queue);
		
		Node * no = (Node *) malloc(sizeof(Node));
		memcpy(no, internalQ->head, sizeof(Node));
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
		
		
		// If there is no worker available the thread waits
		sem_wait(active_workers);
		sem_wait(mutex_shm);
		if (DEBUG) printf("[DISPATCHER]: CONSUMED MESSAGE %s\n", no->data);
		int k;
		for (k = 0; k < sh_var->N_WORKERS; k++) {
			//printf("teste %d\n", sh_var->workers[k].active);
			if (sh_var->workers[k].active == 0) {
				if (DEBUG) printf("====%d\n", k);
				
				char mes[100];
				sprintf(mes, "DISPATCHER: MESSAGE %s SENT FOR PROCESSING ON WORKER %d\n", no->data, k+1);
				write_logfile(mes);
				
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
        	signal(SIGTSTP, SIG_IGN);
			signal(SIGKILL, SIG_IGN);
			signal(SIGTERM, SIG_IGN);
			signal(SIGKILL, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			signal(SIGSTOP, SIG_IGN);
            worker(i + 1);
            exit(0);
        }
    }

    // Create Alerts_Watcher
    if (fork() == 0) {
    	signal(SIGINT, SIG_IGN);
    	signal(SIGTSTP, SIG_IGN);
		signal(SIGKILL, SIG_IGN);
		signal(SIGTERM, SIG_IGN);
		signal(SIGKILL, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGSTOP, SIG_IGN);
        alerts_watcher(0);
        exit(0);
    }
    
	

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
    
    return 0;
}

int main(int argc, char *argv[]) {
	
	//terminates when CTRL-C is pressed
	signal(SIGINT, sigint);
	signal(SIGTSTP, signals_to_ignore);
	signal(SIGKILL, signals_to_ignore);
	signal(SIGTERM, signals_to_ignore);
	signal(SIGKILL, signals_to_ignore);
	signal(SIGQUIT, signals_to_ignore);
	signal(SIGSTOP, signals_to_ignore);
	
	if (DEBUG) printf("pid: %d\n", getpid());

    log = fopen("log.txt", "w");
    fclose(log);

    int failure = 0;

    if (argc != 2) {
        printf("Wrong argc! %d\n", argc);
        return 1;
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
	sem_unlink("A_WORKERS");
	active_workers = sem_open("A_WORKERS", O_CREAT|O_EXCL, 0777, sh_var->N_WORKERS);
	
	
	// Creates the named pipe if it doesn't exist yet
	if ((mkfifo("CONSOLE_PIPE", O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)) {
		write_logfile("CANNOT CREATE CONSOLE_PIPE\n");
		failure = 1;
	}

	if ((mkfifo("SENSOR_PIPE", O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)) {
		write_logfile("CANNOT CREATE SENSOR\n");
		failure = 1;
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
		write_logfile("ERROR CREATING MESSAGE QUEUE\n");
		failure = 1;
	}
	
	// Initialize cond var
	pthread_mutexattr_t attrmutex;
	pthread_condattr_t attrcond;
	
	pthread_mutexattr_init(&attrmutex);
	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
	
	pthread_condattr_init(&attrcond);
	pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
	
	pthread_cond_init(&sh_var->sens_watcher, &attrcond);
	pthread_mutex_init(&sh_var->mutex_cond, &attrmutex);


    write_logfile("HOME_IOT SIMULATOR STARTING\n");
    
    // Function to create Processes and Threads
    if (failure == 0)
        create_procs_threads();
    else 
    	sigint(1);
    	
    //wait(NULL);

    return 0;
}
