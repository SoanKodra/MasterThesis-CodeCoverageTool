#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "../m2-static-lib/coverage.h"

void _init(void) {}
void cov_dump_uart(void);

volatile uint8_t dump_requested = 0;

void USART2_IRQHandler(void) {
    if (USART2->SR & USART_SR_RXNE) {
        uint8_t received = (uint8_t)USART2->DR;
        if (received == 'D') {
            dump_requested = 1;
        }
    }
}

int add(int a, int b) { cov_mark(0); return a + b; }
int subtract(int a, int b) { cov_mark(1); return a - b; }
int unused_function(void) { cov_mark(2); return 0; }

static void TaskApplication(void *pvParameters) {
    (void) pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        add(2, 3);
        subtract(5, 1);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

static void TaskDumpHandler(void *pvParameters) {
    (void) pvParameters;

    for (;;) {
        if (dump_requested) {
            dump_requested = 0;
            uart_puts("=== coverage dump start ===\r\n");
            cov_dump_uart();
            uart_puts("=== coverage dump end ===\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

int main(void) {
    uart_init();
    uart_enable_rx_interrupt();

    xTaskCreate(TaskApplication, "App", 200, NULL, 2, NULL);
    xTaskCreate(TaskDumpHandler, "Dump", 200, NULL, 1, NULL);

    vTaskStartScheduler();

    uart_puts("FATAL: scheduler failed to start\r\n");
    for (;;) {
    }

    return 0;
}
