#ifndef COVERAGE_H
#define COVERAGE_H

#include <stdint.h>

/*
 * Maximale Anzahl an Coverage-Probes, die dieses Bibliotheks-Grundgerüst
 * unterstützt. Bestimmt die Größe der Bitmap in coverage.c.
 * (Erster Prototyp: fester Wert. Später ggf. dynamisch je nach
 * Anzahl der vom Parser erkannten Instrumentierungspunkte.)
 */
#define COV_MAX_PROBES 64

/*
 * Markiert die Probe mit der gegebenen ID als "ausgeführt", indem
 * genau ein Bit in der internen Bitmap gesetzt wird.
 * Wird an jeder instrumentierten Stelle im Zielcode aufgerufen
 * (vgl. COV_MARK-Konzept aus dem Proposal, Abschnitt 10).
 *
 * id: Nummer der Probe, muss < COV_MAX_PROBES sein.
 *     IDs außerhalb des gültigen Bereichs werden stillschweigend ignoriert.
 */
void cov_mark(unsigned int id);

/*
 * Gibt alle aktuell als "ausgeführt" markierten Probe-IDs aus,
 * eine ID pro Zeile, auf stdout.
 * Simuliert hier den UART-Dump zum Host aus dem Zielkonzept;
 * die Ausgabe ist bewusst maschinenlesbar, nicht für Menschen formatiert.
 */
void cov_dump(void);

#endif
