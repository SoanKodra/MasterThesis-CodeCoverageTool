#ifndef F_CPU
#define F_CPU 16000000UL   // muss vor delay.h stehen
#endif

#include <util/delay.h>
#include "uart.h"
#include "../coverage.h"   // cov_mark(), COV_MAX_PROBES

// Deklaration der Funktion aus coverage_dump_uart.c
// (kein eigener Header dafür, da diese Funktion nur hier gebraucht wird)
void cov_dump_uart(void);

// Ein paar Testfunktionen, analog zu eurem PC-Test aus M2:
// add() und subtract() werden aufgerufen, unused_function() bewusst nicht -
// damit entsteht eine echte, sichtbare Coverage-Lücke.
int add(int a, int b) {
    cov_mark(0);
    return a + b;
}

int subtract(int a, int b) {
    cov_mark(1);
    return a - b;
}

void unused_function(void) {
    cov_mark(2);   // wird nie erreicht, absichtlich
}

int main(void) {
    uart_init();

    int result1 = add(3, 4);
    int result2 = subtract(10, 4);
    cov_mark(3);   // Probe für main() selbst

    // Kurze Pause, damit man beim Board-Neustart Zeit hat,
    // "screen" schon geöffnet zu haben, bevor die Ausgabe kommt
    _delay_ms(2000);

    uart_puts("=== coverage dump start ===\r\n");
    cov_dump_uart();
    uart_puts("=== coverage dump end ===\r\n");

    while (1) {
        // Endlosschleife (typisches Embedded-Muster) -
        // hier bewusst leer, Dump ist bereits einmalig erfolgt
    }

    return 0;
}
