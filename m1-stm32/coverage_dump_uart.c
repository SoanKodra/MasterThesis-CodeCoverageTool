#include "../m2-static-lib/coverage.h"   // portabler Kern: COV_MAX_PROBES, cov_is_covered()
#include "uart.h"                          // unser STM32-UART-Transport
#include <stdio.h>                          // fuer sprintf

// STM32-Gegenstueck zu cov_dump() - identisches Prinzip wie bei AVR:
// sendet covered Probe-IDs ueber UART statt printf.
void cov_dump_uart(void)
{
    char buf[8];

    for (unsigned int id = 0; id < COV_MAX_PROBES; id++) {
        if (cov_is_covered(id)) {
            sprintf(buf, "%u\r\n", id);
            uart_puts(buf);
        }
    }
}
