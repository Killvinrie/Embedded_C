/* Compile to assembly only. No main, no simulated ISR, no runnable test. */
unsigned int plain_ready;
volatile unsigned int volatile_ready;

void wait_plain(void)
{
    while (plain_ready == 0u) {
    }
}

void wait_volatile(void)
{
    while (volatile_ready == 0u) {
    }
}
