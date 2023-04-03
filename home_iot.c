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
#include <pthread.h>
#include <time.h>

typedef struct shared_memory{
    int N_WORKERS;
    int QUEUE_SZ;
    int MAX_ALERTS;
    int MAX_KEYS;
    int MAX_SENSORS;
    int teste;

} Shared_var;


Shared_var* sh_var;
int shmid;
sem_t *mutex_shm;
sem_t *mutex_log;

static FILE * log = NULL;

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

int worker(int id) {

    char* mes = malloc(sizeof(char) * (20));
    sprintf(mes, "WORKER %d READY\n", id);
    write_logfile(mes);
    free(mes);

    sem_wait(mutex_shm);
	sh_var->teste++;
    printf("TESTE: %d\n", sh_var->teste);
    sem_post(mutex_shm);

    return 0;
}

int alerts_watcher(int id) {

    write_logfile("PROCESS ALERTS_WATCHER CREATED\n");


    return 0;
}

void * console_reader(void * id_t) {

    write_logfile("THREAD CONSOLE_READER CREATED\n");


    pthread_exit(NULL);
}

void * sensor_reader(void * id_t) {

    write_logfile("THREAD SENSOR_READER CREATED\n");


    pthread_exit(NULL);
}

void * dispatcher(void * id_t) {

    write_logfile("THREAD DISPATCHER CREATED\n");


    pthread_exit(NULL);
}


int system_manager(pid_t pid) {

    pthread_t console_reader_t;
    pthread_t sensor_reader_t;
    pthread_t dispatcher_t;

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

    // Create Console Reader thread
    int cr_id = 1;
    pthread_create(&console_reader_t, NULL, console_reader, &cr_id);

    // Create Sensor Reader thread
    int sr_id = 2;
    pthread_create(&sensor_reader_t, NULL, sensor_reader, &sr_id);

    // Create Dispatcher thread
    int d_id = 3;
    pthread_create(&dispatcher_t, NULL, dispatcher, &d_id);

    // Join Threads
    pthread_join(console_reader_t, NULL);
    pthread_join(sensor_reader_t, NULL);
    pthread_join(dispatcher_t, NULL);


    // Wait for Workers and Alerts_Watcher
    for (i = 0; i < sh_var->N_WORKERS + 1; i++) {
        wait(NULL);
    }

    return 0;
}

int main(int argc, char *argv[]) {

    
    log = fopen("log.txt", "w");
    fclose(log);

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
        return -1;
    }

    sh_var->QUEUE_SZ = QUEUE_SZ;

    fgets(line, 30, config);
    int N_WORKERS = atoi(line);

    if (N_WORKERS < 1) {
        printf("N_WORKERS must be (>=1)\n");
        return -1;
    }
    
    sh_var->N_WORKERS = N_WORKERS;

    fgets(line, 30, config);
    int MAX_KEYS = atoi(line);

    if (MAX_KEYS < 1) {
        printf("MAX_KEYS must be (>=1)\n");
        return -1;
    }

    sh_var->MAX_KEYS = MAX_KEYS;

    fgets(line, 30, config);
    int MAX_SENSORS = atoi(line);

    if (MAX_SENSORS < 1) {
        printf("MAX_SENSORS must be (>=1)\n");
        return -1;
    }

    sh_var->MAX_SENSORS = MAX_SENSORS;

    fgets(line, 30, config);
    int MAX_ALERTS = atoi(line);

    if (MAX_ALERTS < 0) {
        printf("MAX_ALERTS must be (>=0)\n");
        return -1;
    }

    sh_var->MAX_ALERTS = MAX_ALERTS;

    fclose(config);

    sh_var->teste = 0;

    // Create Semaphores
    sem_unlink("MUTEX_SHM");
	mutex_shm = sem_open("MUTEX_SHM", O_CREAT|O_EXCL, 0777, 1);
    sem_unlink("MUTEX_LOG");
	mutex_log = sem_open("MUTEX_LOG", O_CREAT|O_EXCL, 0777, 1);


    pid_t sys_manager = fork();
    if (sys_manager > 0) {
        write_logfile("HOME_IOT SIMULATOR STARTING\n");
    } else if (sys_manager == 0){
        system_manager(getpid());
        exit(0);
    }
	
    wait(NULL);

    write_logfile("HOME_IOT SIMULATOR CLOSING\n");

    // Remove resources
    sem_close(mutex_shm);
	sem_unlink("MUTEX_SHM");
    
    sem_close(mutex_log);
	sem_unlink("MUTEX_LOG");
	
	//shmdt(sh_var);
	//shmctl(shmid, IPC_RMID, NULL);

    return 0;
}