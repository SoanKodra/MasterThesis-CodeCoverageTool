// Treiber für die INSTRUMENTIERTE Version, identischer Ablauf wie baseline_main.c
int add(int a, int b);
int subtract(int a, int b);
int unused_function(void);

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
