// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h> // include POSIX semaphores

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>

typedef struct Node {
    char * data;
    struct Node* next;
    struct Node* prev;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    int count;
} Queue;

typedef struct {
    int id;
    int active;
} Worker;

typedef struct shared_memory{
    int N_WORKERS;
    int QUEUE_SZ;
    int MAX_ALERTS;
    int MAX_KEYS;
    int MAX_SENSORS;
    int teste;
    int terminate;
	
	
	pthread_t console_reader_t;
	pthread_t sensor_reader_t;
	pthread_t dispatcher_t;
	
	Worker * workers;
	
} Shared_var;

Shared_var* sh_var;

Queue* internalQ;

int shmid;
int shwid;
sem_t *mutex_shm;
sem_t *mutex_log;
sem_t *sem_qsize;
sem_t *sem_qcons;




void write_logfile(char * message);

int worker(int id);

int alerts_watcher(int id);
