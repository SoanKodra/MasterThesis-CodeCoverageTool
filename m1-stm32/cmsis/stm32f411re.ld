/* Linker-Script fuer STM32F411RE (Nucleo-F411RE)
 * Flash: 512KB ab 0x08000000, RAM: 128KB ab 0x20000000 */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Anfangswert des Stack-Pointers: Ende des RAM (Stack waechst nach unten) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* Vector Table muss am Anfang des Flash liegen (Adresse 0x08000000) */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Eigentlicher Programmcode */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Konstante Daten (z.B. const-Variablen) */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Initialisierte globale Variablen: liegen im Flash (Ausgangswerte),
     * werden aber beim Start ins RAM kopiert (macht der Startup-Code) */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    /* Nicht initialisierte globale Variablen: nur RAM-Platz reserviert,
     * werden beim Start auf 0 gesetzt (macht der Startup-Code) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
