// Minimalprogramm fuer STM32F411RE: laesst nur eine LED blinken (Nucleo-Boards
// haben eine Onboard-LED auf Pin PA5), analog zum allerersten AVR-Test.
// Kein UART, keine Coverage-Library - nur der Nachweis, dass Code laeuft.

#include "stm32f4xx.h"

// Leerer Stub: newlib's __libc_init_array() erwartet diese Funktion,
// die normalerweise von einem vollen OS-Unterbau kommt. Wir brauchen
// hier keine echte Initialisierung, daher leerer Rumpf.
void _init(void) {}

// Kurze, grobe Wartezeit (busy-loop, keine Timer-Konfiguration noetig
// fuer diesen ersten Test)
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}

int main(void) {
    // GPIOA-Takt einschalten (RCC = Reset and Clock Control -
    // Peripherie ist standardmaessig getaktet AUS, muss explizit aktiviert werden)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA5 als Ausgang konfigurieren (MODER-Register, 2 Bits pro Pin,
    // 01 = General purpose output mode)
    GPIOA->MODER &= ~(3U << (5 * 2));   // Pin 5 Bits erst loeschen
    GPIOA->MODER |=  (1U << (5 * 2));   // dann auf "Output" setzen

    while (1) {
        GPIOA->ODR ^= (1U << 5);   // PA5 togglen (XOR mit sich selbst = umschalten)
        delay(500000);
    }

    return 0;
}
