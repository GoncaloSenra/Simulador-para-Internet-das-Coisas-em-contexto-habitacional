// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include "sys_header.h"

static FILE * log = NULL;

void exit_home_iot(){

	write_logfile("HOME_IOT SIMULATOR WAITING FOR LAST TASKS TO FINISH\n");
	
	// Join Threads
	sem_wait(mutex_shm);
	
	sh_var->terminate=1;
	
    pthread_join(sh_var->console_reader_t, NULL);
    pthread_join(sh_var->sensor_reader_t, NULL);
    pthread_join(sh_var->dispatcher_t, NULL);
	sem_post(mutex_shm);
	
	write_logfile("HOME_IOT SIMULATOR CLOSING\n");
	
    // Remove resources
    sem_close(mutex_shm);
	sem_unlink("MUTEX_SHM");
    
    sem_close(mutex_log);
	sem_unlink("MUTEX_LOG");
	
	shmdt(sh_var);
	shmctl(shmid, IPC_RMID, NULL);

}

void sigint(int signum) { // handling of CTRL-C
	printf("hellooooo\n");
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
    
    // Opens the pipe for reading
	int fd;
	if ((fd = open("CONSOLE_PIPE", O_RDONLY|O_NONBLOCK)) < 0) {
		perror("Cannot open pipe for reading: ");
		exit(0);
	}
	char buffer[1024] = "";
	
	while(1){
		//sem_wait(mutex_shm);
		if (sh_var->terminate == 1){
			write_logfile("THREAD CONSOLE_READER EXITING\n");
			pthread_exit(NULL);
		}
		//sem_post(mutex_shm);
		
	
		long tam = read(fd, buffer, 1024);
		buffer[tam-1] = '\0';
		
		if (strcmp(buffer, "") != 0){
			printf("CONSOLE_PIPE: %s\n", buffer);
			memset(buffer, 0, 1024);
		}
		sleep(2);
	}

    pthread_exit(NULL);
}

void * sensor_reader(void * id_t) {

    write_logfile("THREAD SENSOR_READER CREATED\n");

	// Opens the pipe for reading
	int fd;
	if ((fd = open("SENSOR_PIPE", O_RDONLY|O_NONBLOCK)) < 0) {
		perror("Cannot open pipe for reading: ");
		exit(0);
	}
	char buffer[1024] = "";
	
	while(1){
		
		//sem_wait(mutex_shm);
		if (sh_var->terminate == 1){
			write_logfile("THREAD SENSOR_READER EXITING\n");
			pthread_exit(NULL);
		}
		//sem_post(mutex_shm);
		
		
		long tam = read(fd, buffer, 1024);
		buffer[tam-1] = '\0';
		
		if (strcmp(buffer, "") != 0){
			printf("SENSOR_PIPE: %s\n", buffer);
			memset(buffer, 0, 1024);
		}
	}
	
    pthread_exit(NULL);
}

void * dispatcher(void * id_t) {

    write_logfile("THREAD DISPATCHER CREATED\n");
	
	while(1){
		//sem_wait(mutex_shm);
		if (sh_var->terminate == 1){
			write_logfile("THREAD DISPATCHER EXITING\n");
			pthread_exit(NULL);
		}
		//sem_post(mutex_shm);
	}

}


int create_procs_threads() {

    // Create Workers
    int i;
    for (i = 0; i < sh_var->N_WORKERS; i++) {
        if (fork() == 0) {
            worker(i + 1);
            exit(0);
        }
    }

    // Create Alerts_Watcher
    if (fork() == 0) {
        alerts_watcher(0);
        exit(0);
    }
    
	
	printf("teste\n");
	
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
        printf("Error!\n");
        return -1;
    }

    // Create Shared Memory
    shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT|0777);

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

    fgets(line, 30, config);
    int MAX_KEYS = atoi(line);

    if (MAX_KEYS < 1) {
        printf("MAX_KEYS must be (>=1)\n");
        failure = 1;
    }

    sh_var->MAX_KEYS = MAX_KEYS;

    fgets(line, 30, config);
    int MAX_SENSORS = atoi(line);

    if (MAX_SENSORS < 1) {
        printf("MAX_SENSORS must be (>=1)\n");
        failure = 1;
    }

    sh_var->MAX_SENSORS = MAX_SENSORS;

    fgets(line, 30, config);
    int MAX_ALERTS = atoi(line);

    if (MAX_ALERTS < 0) {
        printf("MAX_ALERTS must be (>=0)\n");
        failure = 1;
    }

    sh_var->MAX_ALERTS = MAX_ALERTS;

    fclose(config);

    sh_var->teste = 0;

    // Create Semaphores
    sem_unlink("MUTEX_SHM");
	mutex_shm = sem_open("MUTEX_SHM", O_CREAT|O_EXCL, 0777, 1);
    sem_unlink("MUTEX_LOG");
	mutex_log = sem_open("MUTEX_LOG", O_CREAT|O_EXCL, 0777, 1);
	
	
	// Creates the named pipe if it doesn't exist yet
	//unlink("CONSOLE_PIPE");
	if ((mkfifo("CONSOLE_PIPE", O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)) {
		perror("Cannot create pipe: ");
		exit(0);
	}
	
	if ((mkfifo("SENSOR_PIPE", O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)) {
		perror("Cannot create pipe: ");
		exit(0);
	}

    write_logfile("HOME_IOT SIMULATOR STARTING\n");
    
    // Function to create Processes and Threads
    if (failure == 0)
        create_procs_threads();
    else 
    	exit_home_iot();
    	
    //wait(NULL);

    return 0;
}
