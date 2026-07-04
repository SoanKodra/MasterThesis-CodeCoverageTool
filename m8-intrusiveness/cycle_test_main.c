// Minimaltest: ruft cov_mark(3) genau einmal auf, für zyklengenaue Messung mit simulavr
extern void cov_mark(unsigned int id);

int main(void) {
    cov_mark(999);
    while (1) {
    }
    return 0;
}
