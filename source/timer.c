
#include "mx/timer.h"



static volatile uint32_t clock_ms = 0;
static volatile uint32_t clock_s = 0;
static volatile uint32_t clock_chrono = 0;



/**
 *  Update clock values
 *
 *  The millisecond counter wraps every ~50 days.
 *       ULONG   / 24 / 60 / 60 / 1000
 *  4294967295ms / 24 / 60 / 60 / 1000  =  49.71 days
 *
 *  After wrapping clock_s is delayed by 295ms. For high accuracy use uint64_t type.
 *
 */
void clock_update(uint32_t ms, uint32_t chrono)
{
    if (ms) {
        clock_s += ((clock_ms % 1000) + ms)/1000;
        clock_ms += ms;
    }

    clock_chrono = chrono;
}


/**
 * Milliseconds getter
 *
 */
uint32_t clock_get_milis()
{
    return clock_ms;
}


/**
 * Seconds getter
 *
 */
uint32_t clock_get_seconds()
{
    return clock_s;
}


/**
 * Chrono getter
 *
 */
uint32_t clock_get_chrono()
{
    return clock_chrono != 0 ? clock_chrono : clock_s;
}
