#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <threads.h>

int do_loop(SemaphoreHandle_t semaphore,
            int *counter,
            const char *src,
            TickType_t timeout)
{
    if (xSemaphoreTake(semaphore, timeout) == pdFALSE)
        return pdFALSE;
    {
        (*counter)++;
        printf("hello world from %s! Count %d\n", src, *counter);
    }
    xSemaphoreGive(semaphore);
    return pdTRUE;
}

void deadlock(void *params) {
    struct deadlock_args *args = (struct deadlock_args *)params;

    args->counter++; // count should be 1 for deadlock start
    printf("Entered deadlock %c\n", args->id);

    xSemaphoreTake(args->first, portMAX_DELAY);
    {
        args->counter++; // count should be 2 if reached
        printf("Holding first lock %c\n", args->id);

        // Delay until other thread grabs its lock
        vTaskDelay(100);

        xSemaphoreTake(args->second, portMAX_DELAY);
        {
            args->counter++; // count should be 3 if reached
            printf("Holding both locks %c\n", args->id);
        }
        xSemaphoreGive(args->second);
    }
    xSemaphoreGive(args->first);
    vTaskSuspend(NULL); // suspend this task to avoid it running again
}

int orphaned_lock(SemaphoreHandle_t semaphore, int *counter) {
    if (xSemaphoreTake(semaphore, 500) == pdFALSE)
        return pdFALSE; // if semaphore is unavailable, return as if unavailable
    {
        (*counter)++;
        if(*counter % 2 == 0) {
            return 0; // return 0 to continue on next iteration, but don't give semaphore back
        }
    }
    xSemaphoreGive(semaphore); // won't be reached when count is even
    return pdTRUE;
}