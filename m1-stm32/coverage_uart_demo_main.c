#include "stm32f4xx.h"
#include "uart.h"
#include "../m2-static-lib/coverage.h"

void _init(void) {}
void cov_dump_uart(void);

static void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}

int add(int a, int b) { cov_mark(0); return a + b; }
int subtract(int a, int b) { cov_mark(1); return a - b; }
int unused_function(void) { cov_mark(2); return 0; }

int main(void) {
    uart_init();

    add(2, 3);
    subtract(5, 1);
    // unused_function() bewusst nicht aufgerufen - Coverage-Luecke wie bei AVR

    delay(4000000);   // kurze Pause, damit du Zeit hast "screen" zu oeffnen

    uart_puts("=== coverage dump start ===\r\n");
    cov_dump_uart();
    uart_puts("=== coverage dump end ===\r\n");

    while (1) {
    }

    return 0;
}
