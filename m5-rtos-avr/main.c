#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>

#include "FreeRTOS.h"
#include "task.h"

#include "../m2-static-lib/coverage.h"
#include "../m2-static-lib/avr/uart.h"

void cov_dump_uart(void);

// Gleiches Muster wie in eurem bisherigen host_command_demo_main.c:
// ISR setzt nur ein Flag, die eigentliche Arbeit passiert im Task.
volatile uint8_t dump_requested = 0;

ISR(USART0_RX_vect) {
    uint8_t received = UDR0;
    if (received == 'D') {
        dump_requested = 1;
    }
}

// Task 1: simuliert die Zielanwendung - ruft wiederholt add()/subtract() auf,
// analog zu eurer bisherigen main()-Logik, jetzt aber als eigener RTOS-Task.
int add(int a, int b) { cov_mark(0); return a + b; }
int subtract(int a, int b) { cov_mark(1); return a - b; }
int unused_function(void) { cov_mark(2); return 0; }

static void TaskApplication(void *pvParameters) {
    (void) pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        add(2, 3);
        subtract(5, 1);
        // unused_function() bewusst nicht aufgerufen - Coverage-Luecke wie gehabt
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

// Task 2: wartet auf das per ISR gesetzte Dump-Flag, fuehrt den Dump aus.
// Getrennter Task statt Polling in der Hauptschleife - das ist der eigentliche
// RTOS-Mehrwert hier: zwei nebenlaeufige Aktivitaeten, sauber getrennt.
static void TaskDumpHandler(void *pvParameters) {
    (void) pvParameters;

    for (;;) {
        if (dump_requested) {
            dump_requested = 0;
            uart_puts("=== coverage dump start ===\r\n");
            cov_dump_uart();
            uart_puts("=== coverage dump end ===\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(50));  // nicht busy-warten, anderen Tasks Zeit geben
    }
}

int main(void) __attribute__((OS_main));

int main(void) {
    uart_init();
    uart_enable_rx_interrupt();

    xTaskCreate(TaskApplication, "App", 200, NULL, 2, NULL);
    xTaskCreate(TaskDumpHandler, "Dump", 200, NULL, 1, NULL);

    vTaskStartScheduler();

    // wird nur erreicht, falls der Scheduler nicht genug Speicher fuer den
    // Idle-Task hatte - sollte bei korrekter Konfiguration nie passieren
    uart_puts("FATAL: scheduler failed to start\r\n");
    for (;;) {
    }

    return 0;
}
