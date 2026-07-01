#include "coverage.h"

/*
 * Testanwendung für das Coverage-Bibliotheks-Grundgerüst.
 * Simuliert eine winzige "Zielanwendung" mit drei Funktionen,
 * jede mit einem eigenen cov_mark()-Aufruf an ihrem Anfang
 * (entspricht später einer instrumentierten Codezeile im echten Target).
 */

int add(int a, int b)
{
    cov_mark(0);
    return a + b;
}

int subtract(int a, int b)
{
    cov_mark(1);
    return a - b;
}

/*
 * Wird absichtlich nie aufgerufen (siehe main()), um eine echte
 * Coverage-Lücke im Report sichtbar zu machen.
 */
int unused_function(void)
{
    cov_mark(2);
    return 0;
}

int main(void)
{
    cov_mark(3);

    add(2, 3);
    subtract(5, 1);
    /* unused_function() wird absichtlich NICHT aufgerufen */

    cov_dump();
    return 0;
}
