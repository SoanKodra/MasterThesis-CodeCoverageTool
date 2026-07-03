#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "../coverage.h"

void cov_dump_uart(void);

// Diese drei Funktionen kommen jetzt NICHT mehr handgeschrieben,
// sondern automatisch instrumentiert aus instrumented_target.c
// (erzeugt von m7-parser/instrument.py) - Deklarationen hier,
// damit main() sie kennt.
int add(int a, int b);
int subtract(int a, int b);
int unused_function(void);

volatile uint8_t dump_requested = 0;

ISR(USART0_RX_vect) {
    uint8_t received = UDR0;
    if (received == 'D') {
        dump_requested = 1;
    }
}

int main(void) {
    uart_init();
    uart_enable_rx_interrupt();

    // Zielanwendung: add und subtract aufrufen,
    // unused_function bewusst NICHT - Coverage-Lücke wie gehabt,
    // diesmal aber automatisch instrumentierter Code
    add(2, 3);
    subtract(5, 1);

    while (1) {
        if (dump_requested) {
            dump_requested = 0;
            uart_puts("=== coverage dump start ===\r\n");
            cov_dump_uart();
            uart_puts("=== coverage dump end ===\r\n");
        }
    }

    return 0;
}
