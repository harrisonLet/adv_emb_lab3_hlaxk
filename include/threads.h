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

int orphaned_lock(SemaphoreHandle_t semaphore, int *counter);

typedef struct {
    SemaphoreHandle_t first;       // lock this task takes first
    SemaphoreHandle_t second;      // lock this task takes second (where it can deadlock)
    SemaphoreHandle_t my_ready;    // this task gives it once it holds `first`
    SemaphoreHandle_t other_ready; // this task waits on it before trying `second`
    int *counter;
    const char *name;
} lock_pair_t;

void grab_two_locks(void *pvParameters);

#endif // THREADS_H