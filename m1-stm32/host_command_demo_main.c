#include "stm32f4xx.h"
#include "uart.h"
#include "../m2-static-lib/coverage.h"

void _init(void) {}
void cov_dump_uart(void);

// "volatile" - Pflicht, da eine ISR schreibt und main() liest
volatile uint8_t dump_requested = 0;

int add(int a, int b) { cov_mark(0); return a + b; }
int subtract(int a, int b) { cov_mark(1); return a - b; }
int unused_function(void) { cov_mark(2); return 0; }

// Ueberschreibt den "weak" Default-Handler aus dem Startup-Code -
// Name muss exakt "USART2_IRQHandler" sein.
void USART2_IRQHandler(void) {
    if (USART2->SR & USART_SR_RXNE) {
        // Lesen von DR loescht automatisch das RXNE-Flag
        // (Pflicht, sonst loest der Interrupt sofort erneut aus)
        uint8_t received = (uint8_t)USART2->DR;
        if (received == 'D') {
            dump_requested = 1;
        }
    }
}

int main(void) {
    uart_init();
    uart_enable_rx_interrupt();

    add(2, 3);
    subtract(5, 1);
    // unused_function() bewusst nicht aufgerufen

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
