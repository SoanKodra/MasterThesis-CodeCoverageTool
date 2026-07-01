#include "coverage.h"

int add(int a, int b)
{
    cov_mark(0); return a + b;
}

int subtract(int a, int b)
{
    cov_mark(1); return a - b;
}

int unused_function(void)
{
    cov_mark(2); return 0;
}

int main(void)
{
    cov_mark(3); add(2, 3);
    cov_mark(4); subtract(5, 1);
    cov_dump();
    cov_mark(5); return 0;
}
