#ifndef UART_H
#define UART_H

// Include-Guard: verhindert doppelte Einbindung dieser Datei,
// falls sie versehentlich mehrfach per #include referenziert wird.

// Initialisiert UART0: setzt Baudrate, aktiviert den Sender,
// legt Datenformat fest. Muss einmalig beim Programmstart aufgerufen werden,
// bevor uart_putchar/uart_puts benutzt werden.
void uart_init(void);

// Sendet ein einzelnes Zeichen über UART0.
// Blockiert (wartet), bis die Hardware bereit zum Senden ist.
void uart_putchar(char c);

// Sendet einen kompletten C-String (nullterminiert) über UART0,
// Zeichen für Zeichen mittels uart_putchar.
void uart_puts(const char *s);

#endif
