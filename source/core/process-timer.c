
#include "mx/core/process-timer.h"
#include "mx/core/scheduler.h"


extern struct scheduler default_scheduler;



/**
 * Start process timer
 *
 */
void process_timer_start(struct process_timer *self, struct process *proc, uint32_t time_ms, msgtype_t ev)
{
    self->process = proc;
    self->event = ev;
    scheduler_timer_start(&default_scheduler, self, time_ms);
}


/**
 * Stop process timer
 *
 */
void process_timer_stop(struct process_timer *self)
{
    scheduler_timer_stop(&default_scheduler, self);
}


/**
 * Check if process timer is running
 */
bool process_timer_running(struct process_timer *self)
{
    return timer_running(&self->timer);
}


/**
 * Handle process timers
 *
 */
void process_timer_handler(void)
{
    scheduler_timer_handler(&default_scheduler);
}
