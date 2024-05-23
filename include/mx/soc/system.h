
#ifndef __MX_SOC_SYSTEM_H_
#define __MX_SOC_SYSTEM_H_


#include <stdint.h>


void system_init(void);

void system_reset(void);
void system_wakeup(void);
void system_hibernate(uint32_t sleep_ms);
void system_wait_for_interrupt(void);


enum wakeup_reason_e
{
    WAKEUP_UNKNOWN,
    WAKEUP_BY_POWER_ON_RESET,   // cold reset
    WAKEUP_BY_BROWN_OUT_RESET,  // power unstable
    WAKEUP_BY_PIN_RESET,
    WAKEUP_BY_WATCHDOG_RESET,
    WAKEUP_BY_SOFTWARE_RESET,

    WAKEUP_BY_RTC,
    WAKEUP_BY_PIN,
    WAKEUP_BY_BUTTON,

    WAKEUP_BY_USER_DEF,         // user defined reason
};

int system_get_wakeup_reason(void);
int system_set_wakeup_reason(int reason);


uint32_t system_get_ticks(void);
uint32_t system_get_clock_ticks(void);
uint32_t system_get_micro_ticks(void);
uint32_t system_get_uptime(void);
uint32_t system_get_wrktime(void);



/**
 * Sleep given milliseconds interval
 *
 */
static inline void system_sleep_ms(uint32_t ms)
{
    uint32_t wakeup = system_get_ticks() + ms;
    while(wakeup > system_get_ticks())
        system_wait_for_interrupt();
}


/**
 * Sleep given seconds interval
 */
static inline void system_sleep(uint32_t s)
{
    system_sleep_ms(s*1000);
}


void system_delay_us(uint32_t us);
void system_delay_ms(uint32_t ms);


#endif // __MX_SOC_SYSTEM_H_
