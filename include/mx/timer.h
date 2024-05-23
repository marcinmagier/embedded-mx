
#ifndef __MX_TIMER_H_
#define __MX_TIMER_H_


#include <stdint.h>
#include <stdbool.h>



void clock_update(uint32_t ms, uint32_t chrono);

uint32_t clock_get_milis();
uint32_t clock_get_seconds();
uint32_t clock_get_chrono();




#define TIMER_MS        0                   // Timer milliseconds
#define TIMER_SEC       (0x1 << 0)          // Timer seconds
#define TIMER_CHRONO    (0x1 << 1)          // Timer seconds respecting hibernation


struct timer
{
    uint32_t start;
    uint32_t interval;
    uint8_t flags;
};




static inline uint32_t timer_tstamp(uint8_t flags)
{
    if (flags & TIMER_SEC)
        return clock_get_seconds();
    if (flags & TIMER_CHRONO)
        return clock_get_chrono();
    return clock_get_milis();
}


static inline void timer_start(struct timer *self, uint8_t flags, uint32_t interval)
{
    self->flags = flags;
    self->start = timer_tstamp(self->flags);
    self->interval = interval;
}


static inline void timer_stop(struct timer *self)
{
    self->start = 0;
    self->flags = 0;
    self->interval = 0;
}


static inline void timer_restart(struct timer *self)
{
    self->start = timer_tstamp(self->flags);
}


static inline bool timer_running(struct timer *self)
{
    return self->interval ? true : false;
}


static inline uint32_t timer_value(struct timer *self)
{
    return timer_tstamp(self->flags) - self->start;
}


static inline uint32_t timer_remaining(struct timer *self)
{
    uint32_t value = timer_value(self);
    return (value < self->interval) ? self->interval - value : 0;
}


static inline bool timer_expired(struct timer *self)
{
    if (self->interval == 0)
        return false;
    if (timer_value(self) < self->interval)
        return false;
    return true;
}


#endif /* __MX_TIMER_H_ */
