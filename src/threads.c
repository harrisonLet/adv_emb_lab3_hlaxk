#include <stdio.h>
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