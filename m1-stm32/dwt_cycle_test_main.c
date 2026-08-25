#include "stm32f4xx.h"
#include "../m2-static-lib/coverage.h"
#include "uart.h"

void _init(void) {}

// DWT-Register sind nicht immer in jedem CMSIS-Header direkt als Makro
// verfuegbar, daher hier explizit als Adressen definiert:
#define DWT_CTRL   (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t*)0xE0001004)
#define DEMCR      (*(volatile uint32_t*)0xE000EDFC)

int main(void) {
    uart_init();

    DEMCR |= (1 << 24);
    DWT_CYCCNT = 0;
    DWT_CTRL |= 1;

    volatile uint32_t start = DWT_CYCCNT;
    cov_mark(3);
    volatile uint32_t end = DWT_CYCCNT;

    uint32_t cycles = end - start;

    char buf[32];
    int i = 0;
    uint32_t n = cycles;
    char tmp[10];
    int t = 0;
    if (n == 0) tmp[t++] = '0';
    while (n > 0) {
        tmp[t++] = '0' + (n % 10);
        n /= 10;
    }
    while (t > 0) buf[i++] = tmp[--t];
    buf[i++] = '\r';
    buf[i++] = '\n';
    buf[i] = '\0';

    // Wiederholt senden statt einmalig, damit genug Zeit bleibt
    // "screen" zu oeffnen bevor die Ausgabe verpasst wird.
    while (1) {
        uart_puts("cycles: ");
        uart_puts(buf);

        for (volatile uint32_t d = 0; d < 2000000; d++) {
            __asm__("nop");
        }
    }

    return 0;
}
