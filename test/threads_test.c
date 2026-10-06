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

    int loop_return = do_loop(semaphore, &counter, "side", portMAX_DELAY);

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

    int loop_return = do_loop(semaphore, &counter, "main", portMAX_DELAY);

    printf("Testing do_loop for main with unavailable semaphore\n\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, counter, "Counter incremented on unavailable semaphore\n");
    TEST_ASSERT_EQUAL_INT_MESSAGE(pdFALSE, loop_return, "Semaphore did not return unavailable status code when unavailable\n");
    printf("OK\n\n");
}


// Spins up two tasks that lock sem_a/sem_b in opposite orders and starts the
// scheduler. Not a pass/fail Unity test: vTaskStartScheduler() never returns.
// The demonstration is watching UART output freeze after both tasks print
// "waiting on second lock..." - that freeze is the deadlock.
void run_deadlock_demo(void) {
    static int counter_a = 0;
    static int counter_b = 0;
    static lock_pair_t a_args;
    static lock_pair_t b_args;

    SemaphoreHandle_t sem_a = xSemaphoreCreateCounting(1, 1);
    SemaphoreHandle_t sem_b = xSemaphoreCreateCounting(1, 1);
    SemaphoreHandle_t a_ready = xSemaphoreCreateCounting(1, 0);
    SemaphoreHandle_t b_ready = xSemaphoreCreateCounting(1, 0);

    a_args = (lock_pair_t){ sem_a, sem_b, a_ready, b_ready, &counter_a, "A" };
    b_args = (lock_pair_t){ sem_b, sem_a, b_ready, a_ready, &counter_b, "B" }; // reversed order

    printf("Starting deadlock demo: A takes sem_a then sem_b, B takes sem_b then sem_a\n");

    xTaskCreate(grab_two_locks, "LockerA", configMINIMAL_STACK_SIZE, &a_args, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(grab_two_locks, "LockerB", configMINIMAL_STACK_SIZE, &b_args, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler(); // never returns
}

int main (void)
{
    stdio_init_all();
    while (1) {
        sleep_ms(5000); // Give time for TTY to attach.
        printf("Start tests\n");
        UNITY_BEGIN();
        RUN_TEST(test_smoke_string);
        RUN_TEST(test_main_thread_available);
        RUN_TEST(test_main_thread_unavailable);
        RUN_TEST(test_side_thread_available);
        RUN_TEST(test_side_thread_unavailable);
        sleep_ms(5000);
        UNITY_END();

        run_deadlock_demo();
    }
}
