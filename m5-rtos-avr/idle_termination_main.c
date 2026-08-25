#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "../m2-static-lib/coverage.h"

void vApplicationIdleHook(void);

void cov_dump_uart(void);

volatile uint8_t already_dumped = 0;

int add(int a, int b) { cov_mark(0); return a + b; }
int subtract(int a, int b) { cov_mark(1); return a - b; }
int unused_function(void) { cov_mark(2); return 0; }

// FreeRTOS ruft diese Funktion automatisch auf, wann immer der
// Idle-Task laeuft (d.h. kein anderer Task etwas zu tun hat) -
// Proposal Abschnitt 10.2, Strategie 3 "Idle-Erkennung".
void vApplicationIdleHook(void) {
    if (!already_dumped) {
        already_dumped = 1;
        uart_puts("=== coverage dump start (idle) ===\r\n");
        cov_dump_uart();
        uart_puts("=== coverage dump end ===\r\n");
    }
}

static void TaskApplication(void *pvParameters) {
    (void) pvParameters;

    add(2, 3);
    subtract(5, 1);
    // Kein Delay-Loop noetig - Task beendet sich, danach wird der
    // Idle-Task automatisch aktiv, sobald nichts mehr zu tun ist.
    vTaskDelete(NULL);
}

int main(void) {
    uart_init();

    xTaskCreate(TaskApplication, "App", 200, NULL, 2, NULL);

    vTaskStartScheduler();

    uart_puts("FATAL: scheduler failed to start\r\n");
    for (;;) {
    }

    return 0;
}
