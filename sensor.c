// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>

sem_t *mutex_pipe;
int num_msg = 0;

void sigint()
{
    sem_close(mutex_pipe);
    printf("\nSensor terminating!\n");
    exit(0);
}

void stats()
{
    printf("\nNumber of messages: %d\n", num_msg);
}

int main(int argc, char *argv[])
{
    int i;

    signal(SIGINT, sigint);
    signal(SIGTSTP, stats);

    if (argc != 6)
    {
        printf("%d\n", argc);
        printf("Error!\n");
        printf("sensor {identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {valor inteiro mínimo a ser enviado} {valor inteiro máximo a ser enviado}\n");
        return -1;
    }

    if (strlen(argv[1]) < 3 || strlen(argv[1]) > 32)
    {
        printf("ID wrong size!\n");
    }

    for (i = 0; i < strlen(argv[1]); i++)
    {
        if (!(isalnum(argv[1][i]) || argv[1][i] == '_'))
        {
            printf("ID must be alphanumeric!\n");
            return -1;
        }
    }

    if (strlen(argv[3]) < 3 || strlen(argv[3]) > 32)
    {
        printf("CHAVE wrong size!\n");
    }

    for (i = 0; i < strlen(argv[3]); i++)
    {
        if (!(isalnum(argv[3][i]) || argv[3][i] == '_'))
        {
            printf("CHAVE must be alphanumeric!\n");
            return -1;
        }
    }

    // Verifiar se intervalo de segundos é um número interiro
    for (i = 0; i < strlen(argv[2]); i++)
    {
        if (!isdigit(argv[2][i]))
        {
            printf("Intervalo de segundos must be a number!\n");
            return -1;
        }
    }

    // Verifiar se intervalo de segundos é >= 0
    int sec = atoi(argv[2]);
    if (sec < 0)
    {
        printf("Intervalo de segundos must be a number!\n");
    }

    // Verifiar se Min é um número interiro
    for (i = 0; i < strlen(argv[4]); i++)
    {
        if (!isdigit(argv[4][i]))
        {
            printf("Min must be a number!\n");
            return -1;
        }
    }

    // Verifiar se Max é um número interiro
    for (i = 0; i < strlen(argv[5]); i++)
    {
        if (!isdigit(argv[5][i]))
        {
            printf("Max must be a number!\n");
            return -1;
        }
    }

    int min = atoi(argv[4]);
    int max = atoi(argv[5]);

    if (min > max)
    {
        printf("Max must be higher then min\n");
    }

    // Opens the pipe for writing
    int fd;
    if ((fd = open("SENSOR_PIPE", O_WRONLY | O_NONBLOCK)) < 0)
    {
        perror("Cannot open pipe for writing: ");
        exit(0);
    }

    char buffer[1024] = "";

    // Create Semaphore
    sem_unlink("MUTEX_SENSOR_PIPE");
    mutex_pipe = sem_open("MUTEX_SENSOR_PIPE", O_CREAT | O_EXCL, 0777, 1);

    srand(time(NULL));
    int errPipe = 1;
    while (1)
    {

        int num = (rand() % (max - min + 1)) + min;

        printf("%s - %d\n", argv[3], num);

        sprintf(buffer, "%s#%s#%d", argv[1], argv[3], num);

        sem_wait(mutex_pipe);
        // printf("aqui1\n");
        errPipe = write(fd, &buffer, 1024);
        sem_post(mutex_pipe);

        // printf("status code : %d\n", errPipe);

        if (errPipe == -1)
        {
            printf("ERROR: pipe does not exist\n");
            sigint();
        }

        memset(buffer, 0, 1024);
        num_msg++;

        sleep(sec);
    }
}
