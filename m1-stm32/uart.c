#include "stm32f4xx.h"
#include "uart.h"

// Nucleo-F411RE: USART2 ist werkseitig mit dem ST-LINK Virtual COM Port
// verbunden (PA2=TX, PA3=RX), kein externer USB-Seriell-Chip noetig
// (anders als beim CH340 auf dem Mega2560).
#define UART_BAUD 115200

void uart_init(void) {
    // Takt fuer GPIOA und USART2 einschalten - beide sind ab Werk
    // deaktiviert (Stromsparen), muessen explizit aktiviert werden.
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // PA2 und PA3 auf "Alternate Function" umschalten (MODER = 10),
    // statt normaler GPIO-Nutzung (00 = Input, 01 = Output).
    GPIOA->MODER &= ~((3U << (2*2)) | (3U << (3*2)));
    GPIOA->MODER |=  ((2U << (2*2)) | (2U << (3*2)));

    // Festlegen WELCHE alternative Funktion genau - USART2 ist auf
    // Funktion "AF7" (aus dem Datenblatt, Alternate Function Mapping Tabelle).
    // AFR[0] ist zustaendig fuer Pins 0-7, je 4 Bit pro Pin.
    GPIOA->AFR[0] &= ~((0xFU << (2*4)) | (0xFU << (3*4)));
    GPIOA->AFR[0] |=  ((7U   << (2*4)) | (7U   << (3*4)));

    // Baudrate: USART-Baudrate-Register erwartet einen Wert basierend
    // auf der APB1-Taktfrequenz. Nucleo-F411RE laeuft mit 16 MHz
    // HSI (internem Takt) direkt nach Reset, ohne PLL-Konfiguration.
    // BRR = f_APB1 / Baudrate (vereinfachte Formel fuer Standardfall)
    USART2->BRR = 16000000UL / UART_BAUD;

    // Sender (TE) und USART selbst (UE) aktivieren
    USART2->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

void uart_putchar(char c) {
    // TXE (Transmit Data Register Empty) im Status-Register (SR) abwarten -
    // analog zum UDRE0-Polling bei AVR.
    while (!(USART2->SR & USART_SR_TXE)) {
        ;
    }
    USART2->DR = c;
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putchar(*s);
        s++;
    }
}
