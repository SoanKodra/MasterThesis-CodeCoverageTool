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

