#ifndef THREADS_H
#define THREADS_H

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

int do_loop(SemaphoreHandle_t semaphore,
            int *counter,
            const char *src,
            TickType_t timeout);

#endif // THREADS_H