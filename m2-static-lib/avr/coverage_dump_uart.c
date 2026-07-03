#include "../coverage.h"   // portabler Kern: COV_MAX_PROBES, cov_is_covered()
#include "uart.h"          // unser UART-Transport von heute
#include <stdio.h>          // für sprintf, um die Zahl in Text umzuwandeln

/*
 * AVR-spezifisches Gegenstück zu cov_dump(): statt printf (zu schwergewichtig
 * für AVR, siehe Proposal Abschnitt 3 "Begrenzte Ressourcen") wird hier
 * jede covered Probe-ID als Textzeile über UART gesendet - der eigentliche
 * "UART-Transport"-Baustein aus Proposal Abschnitt 10.3.
 */
void cov_dump_uart(void)
{
    char buf[8];   // Puffer für die Textdarstellung einer ID, z.B. "63\r\n"

    for (unsigned int id = 0; id < COV_MAX_PROBES; id++) {
        if (cov_is_covered(id)) {
            sprintf(buf, "%u\r\n", id);
            uart_puts(buf);
        }
    }
}
