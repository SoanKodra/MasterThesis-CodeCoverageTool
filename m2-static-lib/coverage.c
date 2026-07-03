#include "coverage.h"
#include <stdio.h>
#ifdef __AVR__
#include <util/atomic.h>   // stellt ATOMIC_BLOCK bereit, AVR-spezifisch
#endif

/*
 * Interne Bitmap: 1 Bit pro Probe statt eines vollen Zählers (wie bei Gcov).
 * Größe in Bytes = aufgerundet COV_MAX_PROBES / 8.
 * "static" = nur innerhalb dieser Datei sichtbar; von außen darf
 * nur über cov_mark() / cov_dump() zugegriffen werden, nicht direkt
 * auf das Array.
 */
static uint8_t bitmap[(COV_MAX_PROBES + 7) / 8];

void cov_mark(unsigned int id)
{
    if (id >= COV_MAX_PROBES) {
        return;
    }

#ifdef __AVR__
    // ATOMIC_BLOCK deaktiviert kurzzeitig Interrupts, führt den Block aus,
    // stellt den vorherigen Interrupt-Zustand danach wieder her.
    // Verhindert, dass eine ISR mitten im Bit-Setzen dazwischenfunkt.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        bitmap[id / 8] |= (uint8_t)(1u << (id % 8));
    }
#else
    bitmap[id / 8] |= (uint8_t)(1u << (id % 8));
#endif
}

int cov_is_covered(unsigned int id)
{
    /* Wie in cov_mark(): ungültige IDs werden nicht als Fehler behandelt,
     * sondern einfach als "nicht covered" gewertet. */
    if (id >= COV_MAX_PROBES) {
        return 0;
    }
    /* Gleiche Bit-Extraktion wie in cov_dump(), nur jetzt als eigenständige,
     * wiederverwendbare Abfrage statt fest in eine printf-Schleife eingebaut. */
    return (bitmap[id / 8] >> (id % 8)) & 1;
}

void cov_dump(void)
{
    /* Nur die IDs ausgeben, deren Bit gesetzt ist - keine Formatierung,
     * keine Datei-/Zeileninformation. Das entspricht bewusst dem, was
     * später über UART zum Host geschickt würde: das Target kennt nur
     * nackte Probe-IDs, keine Dateinamen (siehe Konzept, Abschnitt 10.1). */
    for (unsigned int id = 0; id < COV_MAX_PROBES; id++) {
        int covered = (bitmap[id / 8] >> (id % 8)) & 1;
        if (covered) {
            printf("%u\n", id);
        }
    }
}
