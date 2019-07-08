#ifndef SEMAFOROS_H
#define SEMAFOROS_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void P(pthread_mutex_t* mutex);

void V(pthread_mutex_t* mutex);

#endif // SEMAFOROS_H
