#ifndef F_CPU
#define F_CPU 16000000UL   // muss VOR delay.h stehen, sonst falsche Wartezeiten
#endif

#include <util/delay.h>   // stellt _delay_ms() bereit
#include "uart.h"         // unsere eigenen UART-Funktionen

int main(void) {
    uart_init();   // UART einmalig einrichten (Baudrate, Sender an, Format)

    while (1) {                    // Endlosschleife: typisches Embedded-Muster
        uart_puts("hello from mega2560\r\n");
        // \r\n statt nur \n: "Carriage Return" + "Line Feed" -
        // manche Terminal-Programme brauchen beides für einen sauberen Zeilenumbruch

        _delay_ms(1000);            // 1 Sekunde warten, damit es nicht spammt
    }

    return 0;   // wird nie erreicht (Endlosschleife), aber gehört zur main()-Signatur dazu
}
