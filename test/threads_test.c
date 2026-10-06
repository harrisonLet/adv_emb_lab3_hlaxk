#include <stdio.h>
#include <pico/stdlib.h>
#include <stdint.h>
#include <unity.h>
#include "unity_config.h"
#include <FreeRTOS.h>
#include <semphr.h>
#include <threads.h>

SemaphoreHandle_t semaphore;
int counter;

void setUp(void) {}

void tearDown(void) {}


void test_smoke_string(void) {
    const char *expected = "hello unity";
    const char *actual = "hello unity";

    printf("Smoke test: UART/Unity pipeline is alive\n");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, "Smoke test string mismatch\n");
}

void test_side_thread_available(void) {
    semaphore = xSemaphoreCreateCounting(1, 1);
    counter = 0;

    int loop_return = do_loop(semaphore, &counter, "side", portMAX_DELAY);

    printf("Testing do_loop for side with available semaphore\n\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, counter, "Counter did not increment on available semaphore\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdTRUE, loop_return, "Semaphore did not return available status code when available\n");
    printf("OK\n\n");
}

void test_side_thread_unavailable(void) {
    semaphore = xSemaphoreCreateCounting(1, 1);
    counter = 0;

    // Semaphore is unavailable
    xSemaphoreTake(semaphore, portMAX_DELAY);

    int loop_return = do_loop(semaphore, &counter, "side", 0);

    printf("Testing do_loop for side with unavailable semaphore\n\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, counter, "Counter incremented on unavailable semaphore\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdFALSE, loop_return, "Semaphore did not return unavailable status code when unavailable\n");
    printf("OK\n\n");
}

void test_main_thread_available(void) {
    semaphore = xSemaphoreCreateCounting(1, 1);
    counter = 0;

    int loop_return = do_loop(semaphore, &counter, "main", portMAX_DELAY);

    printf("Testing do_loop for main with available semaphore\n\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, counter, "Counter did not incremenet on available semaphore\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdTRUE, loop_return, "Semaphore did not return available status code when available\n");
    printf("OK\n\n");
}

void test_main_thread_unavailable(void) {
    semaphore = xSemaphoreCreateCounting(1, 1);
    counter = 0;

    // Semaphore is unavailable
    xSemaphoreTake(semaphore, portMAX_DELAY);

    int loop_return = do_loop(semaphore, &counter, "main", 0);

    printf("Testing do_loop for main with unavailable semaphore\n\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, counter, "Counter incremented on unavailable semaphore\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdFALSE, loop_return, "Semaphore did not return unavailable status code when unavailable\n");
    printf("OK\n\n");
}

// Deadlock testing
void test_deadlock(void) {
    TaskHandle_t deadlock_a, deadlock_b, deadlock_thread;
    SemaphoreHandle_t first = xSemaphoreCreateCounting(1, 1);
    SemaphoreHandle_t second = xSemaphoreCreateCounting(1, 1);

    struct deadlock_args task_a = {first, second, 0, 'a'};
    struct deadlock_args task_b = {second, first, 10, 'b'}; 

    BaseType_t status_a = xTaskCreate(deadlock, "Deadlock A", configMINIMAL_STACK_SIZE, (void *)&task_a, (tskIDLE_PRIORITY + 5UL) - 1UL, &deadlock_a);
    BaseType_t status_b = xTaskCreate(deadlock, "Deadlock B", configMINIMAL_STACK_SIZE, (void *)&task_b, (tskIDLE_PRIORITY + 5UL) - 1UL, &deadlock_b);
    
    printf("Threads created.\n");
    vTaskDelay(1000); // Allow time for threads to run
    printf("1000 ticks later...\n");

    // Both threads should have taken a semaphore,
    // incremented their respective count twice, 
    // and now be stuck.
    TEST_ASSERT_EQUAL_INT_MESSAGE(uxSemaphoreGetCount(first), 0, "First semaphore is available, when it should be taken.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(uxSemaphoreGetCount(second), 0, "Second semaphore is available, when it should be taken.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, task_a.counter, "Thread A did not increment its counter twice.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(12, task_b.counter, "Thread B did not increment its counter twice.");

    vTaskDelete(deadlock_a);
    vTaskDelete(deadlock_b);
    printf("Killed threads.\n");
}

void test_orphaned(void){
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);
    int counter = 0;

    int first_call = orphaned_lock(semaphore, &counter);
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdTRUE, first_call, "Orphaned_lock did not return pdTRUE when count was odd.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, counter, "Counter was not incremented.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, uxSemaphoreGetCount(semaphore), "Semaphore was not released when count was odd.");

    int second_call = orphaned_lock(semaphore, &counter);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, second_call, "Orphaned_lock did not return 0 when count was even.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, counter, "Counter was not incremented.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, uxSemaphoreGetCount(semaphore), "Semaphore was released when count was even.");

    int third_call = orphaned_lock(semaphore, &counter);
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdFALSE, third_call, "Orphaned_lock did not return pdFALSE on an orphaned semaphore.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, counter, "Counter incremented without holding the semaphore.");
}

void test_orphaned_deadlock(void) {
    TaskHandle_t orphan;
    struct orphaned_args args = {xSemaphoreCreateCounting(1, 1), 0};

    xTaskCreate(orphaned_thread, "Orphaned", configMINIMAL_STACK_SIZE, (void *)&args, (tskIDLE_PRIORITY + 5UL) - 1UL, &orphan);

    printf("Orphaned thread created.\n");
    vTaskDelay(1000); // Allow time for the thread to orphan the lock
    vTaskSuspend(orphan);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, uxSemaphoreGetCount(args.semaphore), "Semaphore is available, when it should be orphaned.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, args.counter, "Thread kept incrementing after orphaning the lock.");

    vTaskDelete(orphan);
    vSemaphoreDelete(args.semaphore);
}

void test_unorphaned(void){
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);
    int counter = 0;

    int first_call = unorphaned_lock(semaphore, &counter);
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdTRUE, first_call, "Unorphaned_lock did not return pdTRUE when count was odd.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, counter, "Counter was not incremented.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, uxSemaphoreGetCount(semaphore), "Semaphore was not released when count was odd.");

    int second_call = unorphaned_lock(semaphore, &counter);
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdTRUE, second_call, "Unorphaned_lock did not return pdTRUE when count was even.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, counter, "Counter was not incremented.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, uxSemaphoreGetCount(semaphore), "Semaphore was not released when count was even.");

    int third_call = unorphaned_lock(semaphore, &counter);
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdTRUE, third_call, "Unorphaned_lock could not take the semaphore after an even count.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, counter, "Counter was not incremented.");

    vSemaphoreDelete(semaphore);
}

void test_unorphaned_no_deadlock(void) {
    TaskHandle_t unorphan;
    struct orphaned_args args = {xSemaphoreCreateCounting(1, 1), 0};

    xTaskCreate(unorphaned_thread, "Unorphaned", configMINIMAL_STACK_SIZE, (void *)&args, (tskIDLE_PRIORITY + 5UL) - 1UL, &unorphan);

    printf("Unorphaned thread created.\n");
    vTaskDelay(1000); // Allow time for the thread to loop past an even count many times
    vTaskSuspend(unorphan);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, uxSemaphoreGetCount(args.semaphore), "Semaphore is held, when it should be released.");
    TEST_ASSERT_GREATER_THAN_INT_MESSAGE(2, args.counter, "Thread stopped incrementing, so it deadlocked.");

    vTaskDelete(unorphan);
    vSemaphoreDelete(args.semaphore);
}

void runner_thread(void *params) {
    while(1){
        printf("Start tests\n");
        UNITY_BEGIN();
        RUN_TEST(test_smoke_string);
        RUN_TEST(test_main_thread_available);
        RUN_TEST(test_main_thread_unavailable);
        RUN_TEST(test_side_thread_available);
        RUN_TEST(test_side_thread_unavailable);
        RUN_TEST(test_deadlock);
        RUN_TEST(test_orphaned);
        RUN_TEST(test_orphaned_deadlock);
        RUN_TEST(test_unorphaned);
        RUN_TEST(test_unorphaned_no_deadlock);
        UNITY_END();
        vTaskDelay(10000);
    }

}

int main (void)
{
    stdio_init_all();
    xTaskCreate(runner_thread, "TestRunner", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 5UL, NULL);
    vTaskStartScheduler();
    return 0;
}
