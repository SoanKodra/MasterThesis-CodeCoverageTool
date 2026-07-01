#include <avr/io.h>

int main(void)
{
    volatile int x = 42;
    while (1) {
        x++;
    }
}
