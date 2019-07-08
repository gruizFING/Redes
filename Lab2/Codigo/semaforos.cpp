#include "semaforos.h"


void P(pthread_mutex_t* mutex)
{
    if(pthread_mutex_lock(mutex) != 0)
    {
        perror("ERROR: fallo al realizar P sobre un semaforo");
    }
}

void V(pthread_mutex_t* mutex)
{
    if(pthread_mutex_unlock(mutex) != 0)
    {
        perror("ERROR: fallo al realizar V sobre un semaforo");
    }
}
