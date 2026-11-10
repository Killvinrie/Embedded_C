#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned int next_id(void)
{
    static unsigned int id;
    return ++id;
}

static inline int square_i(int x)
{
    return x * x; /* Test inputs are small enough to avoid signed overflow. */
}

int main(void)
{
    assert(next_id() == 1u);
    assert(next_id() == 2u);

    int a = 10;
    int b = 20;
    const int *view = &a;
    assert(*view == 10);
    a = 11;
    assert(*view == 11);
    view = &b;
    assert(*view == 20);

    int *const fixed_pointer = &a;
    *fixed_pointer = 30;
    assert(a == 30);

    int i = 3;
    int result = square_i(i++);
    assert(i == 4 && result == 9);

    int *owned = malloc(sizeof *owned);
    if (owned == NULL) {
        fputs("allocation failed\n", stderr);
        return EXIT_FAILURE;
    }
    *owned = 42;
    assert(*owned == 42);
    free(owned);
    owned = NULL;
    free(owned); /* Explicitly permitted: free(NULL) has no effect. */

    puts("PASS: static state, const views, inline argument, allocation ownership");
    return EXIT_SUCCESS;
}
