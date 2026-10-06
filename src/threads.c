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

void grab_two_locks(void *pvParameters)
{
    lock_pair_t *p = (lock_pair_t *)pvParameters;

    xSemaphoreTake(p->first, portMAX_DELAY);
    printf("%s: took first lock\n", p->name);
    (*p->counter)++;

    // Barrier: don't reach for the second lock until we know the other task
    // is already holding its first one too, so the deadlock is deterministic
    // instead of a race.
    xSemaphoreGive(p->my_ready);
    xSemaphoreTake(p->other_ready, portMAX_DELAY);

    printf("%s: waiting on second lock...\n", p->name);
    xSemaphoreTake(p->second, portMAX_DELAY); // never returns: the other task holds this

    printf("%s: took second lock\n", p->name);
    (*p->counter)++;

    xSemaphoreGive(p->second);
    xSemaphoreGive(p->first);
    vTaskDelete(NULL);
}