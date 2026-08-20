#include "stm32f4xx.h"
#include "uart.h"

void _init(void) {}

// Grobe Wartezeit per Busy-Loop (kein Timer noetig fuer diesen Test)
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}

int main(void) {
    uart_init();

    while (1) {
        uart_puts("hello from stm32f411re\r\n");
        delay(2000000);
    }

    return 0;
}
