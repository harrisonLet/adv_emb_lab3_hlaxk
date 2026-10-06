#ifndef THREADS_H
#define THREADS_H

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

struct deadlock_args {
    SemaphoreHandle_t first;
    SemaphoreHandle_t second;
    int counter;
    char id;
};

int do_loop(SemaphoreHandle_t semaphore,
            int *counter,
            const char *src,
            TickType_t timeout);
            
void deadlock(void *params);

#endif // THREADS_H