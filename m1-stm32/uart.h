#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putchar(char c);
void uart_puts(const char *s);
void uart_enable_rx_interrupt(void);

#endif
