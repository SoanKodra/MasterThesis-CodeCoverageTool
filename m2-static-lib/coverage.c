#include "coverage.h"
#include <stdio.h>

#ifdef __AVR__
#include <util/atomic.h>   // stellt ATOMIC_BLOCK bereit, AVR-spezifisch
#endif

/*
 * Interne Bitmap: 1 Bit pro Probe statt eines vollen Zaehlers (wie bei Gcov).
 * Groesse in Bytes = aufgerundet COV_MAX_PROBES / 8.
 * "static" = nur innerhalb dieser Datei sichtbar; von aussen darf
 * nur ueber cov_mark() / cov_dump() zugegriffen werden, nicht direkt
 * auf das Array.
 */
static uint8_t bitmap[(COV_MAX_PROBES + 7) / 8];

void cov_mark(unsigned int id)
{
    /* Ungueltige IDs (ausserhalb der Bitmap-Groesse) werden ignoriert,
     * statt z.B. Speicher ausserhalb des Arrays zu beschreiben. */
    if (id >= COV_MAX_PROBES) {
        return;
    }

#ifdef __AVR__
    /* ATOMIC_BLOCK deaktiviert kurzzeitig Interrupts, fuehrt den Block aus,
     * stellt den vorherigen Interrupt-Zustand danach wieder her.
     * Verhindert, dass eine ISR mitten im Bit-Setzen dazwischenfunkt. */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        bitmap[id / 8] |= (uint8_t)(1u << (id % 8));
    }
#elif defined(__arm__)
    /* Kein CMSIS-Header noetig - reine Inline-Assembler-Befehle auf das
     * PRIMASK-Register funktionieren auf jedem Cortex-M-Kern und halten
     * den portablen Kern frei von STM32-spezifischen Includes
     * (vgl. Proposal Abschnitt 10.3).
     * Ablauf: aktuellen Interrupt-Zustand (PRIMASK) sichern, Interrupts
     * sperren (CPSID), Bit setzen, urspruenglichen Zustand wiederherstellen -
     * verschachtelungssicher, analog zu AVRs ATOMIC_RESTORESTATE. */
    uint32_t primask;
    __asm__ volatile ("mrs %0, primask" : "=r" (primask));
    __asm__ volatile ("cpsid i" ::: "memory");

    bitmap[id / 8] |= (uint8_t)(1u << (id % 8));

    __asm__ volatile ("msr primask, %0" :: "r" (primask) : "memory");
#else
    /* PC-Build (kein Embedded-Target): keine Interrupt-Sperrung noetig,
     * dient hier nur der Kompilierbarkeit fuer Tests auf dem Host. */
    bitmap[id / 8] |= (uint8_t)(1u << (id % 8));
#endif
}

void cov_dump(void)
{
    /* Nur die IDs ausgeben, deren Bit gesetzt ist - keine Formatierung,
     * keine Datei-/Zeileninformation. Das entspricht bewusst dem, was
     * spaeter ueber UART zum Host geschickt wuerde: das Target kennt nur
     * nackte Probe-IDs, keine Dateinamen (siehe Konzept, Abschnitt 10.1). */
    for (unsigned int id = 0; id < COV_MAX_PROBES; id++) {
        int covered = (bitmap[id / 8] >> (id % 8)) & 1;
        if (covered) {
            printf("%u\n", id);
        }
    }
}

int cov_is_covered(unsigned int id)
{
    /* Wie in cov_mark(): ungueltige IDs werden nicht als Fehler behandelt,
     * sondern einfach als "nicht covered" gewertet. */
    if (id >= COV_MAX_PROBES) {
        return 0;
    }
    /* Gleiche Bit-Extraktion wie in cov_dump(), nur jetzt als eigenstaendige,
     * wiederverwendbare Abfrage statt fest in eine printf-Schleife eingebaut. */
    return (bitmap[id / 8] >> (id % 8)) & 1;
}
