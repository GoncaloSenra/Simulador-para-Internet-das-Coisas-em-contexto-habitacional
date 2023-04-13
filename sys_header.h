// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h> // include POSIX semaphores

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

void write_logfile(char * message);

int worker(int id);

int alerts_watcher(int id);