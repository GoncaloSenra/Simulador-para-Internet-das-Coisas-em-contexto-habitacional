// Gonçalo Senra    2020213750
// Rui Coelho       2021235407

#include "sys_header.h"

int worker(int id)
{

    char *mes = malloc(sizeof(char) * (20));
    sprintf(mes, "WORKER %d READY\n", id);
    write_logfile(mes);
    free(mes);

    sem_wait(mutex_shm);
    sh_var->teste++;
    printf("TESTE: %d\n", sh_var->teste);
    sem_post(mutex_shm);

    return 0;
}
