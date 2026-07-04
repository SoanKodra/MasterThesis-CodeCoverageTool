// Minimaltest: misst die Dauer von cov_dump_uart() bei 3 gesetzten Bits
extern void cov_mark(unsigned int id);
extern void cov_dump_uart(void);
extern void uart_init(void);

int main(void) {
    uart_init();
    cov_mark(0);
    cov_mark(1);
    cov_mark(3);
    cov_dump_uart();
    while (1) {
    }
    return 0;
}
