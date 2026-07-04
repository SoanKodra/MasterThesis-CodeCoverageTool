// Baseline OHNE Coverage-Instrumentierung, zum Größenvergleich (M8, FF2)
int add(int a, int b) { return a + b; }
int subtract(int a, int b) { return a - b; }
int unused_function(void) { return 0; }

volatile int input_a = 2;
volatile int input_b = 3;

int main(void) {
    volatile int result1 = add(input_a, input_b);
    volatile int result2 = subtract(input_a, input_b);
    (void)result1;
    (void)result2;

    while (1) {
    }
    return 0;
}
