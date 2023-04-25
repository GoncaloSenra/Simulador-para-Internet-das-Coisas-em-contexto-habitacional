// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h> // include POSIX semaphores

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
	pid_t pid; 
    int id;
    int active;
} Worker;

typedef struct { 
    char * id;
    int last_value;
    int max;
    int min;
    double avg;
    int count;
} Key;

typedef struct {
	char * id;
	
} Sensor;

typedef struct {
	char * id;
	char * key;
	int min;
	int max;
} Alert;

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
	Sensor * sensors;
	Key * keys;
	Alert * alerts;
	
} Shared_var;

// Shared memory
Shared_var* sh_var;

// Unnamed pipes file decriptors
int ** channels;

// Internal Queue
Queue* internalQ;

// IDs of the different blocks of shared memory
int shmid; //main block
int shwid; // workers
int shsid; // sensors
int shkid; // keys
int shaid; // alerts

// Semaphores
sem_t *mutex_shm;
sem_t *mutex_log;
sem_t *sem_qsize;
sem_t *sem_qcons;



void write_logfile(char * message);

int worker(int id);

int alerts_watcher(int id);
