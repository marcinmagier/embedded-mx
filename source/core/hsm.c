
#include "mx/core/hsm.h"
#include "mx/core/message.h"

#ifdef DEBUG_HSM
  #include "mx/trace.h"
#endif

#ifdef TRACE_HSM
  extern void hsm_trace_state(int state);
#else
  #define hsm_trace_state(state)
#endif





/**
 * Reset hsm timer.
 *
 */
void hsm_timer_init(struct hsm *hsm)
{
    hsm->timer.counter = 0;
    hsm->timer.mask = HSM_TIMER_DISABLED;
}


/**
 * Reset hsm timer.
 *
 */
void hsm_timer_reset(struct hsm *hsm)
{
    hsm->timer.counter = 0;
}


/**
 * Enable hsm timers defined by the mask.
 *
 */
void hsm_timer_enable(struct hsm *hsm, unsigned short mask)
{
    hsm->timer.mask |= mask;

}


/**
 * Disable hsm timers defined by the mask.
 *
 */
void hsm_timer_disable(struct hsm *hsm, unsigned short mask)
{
    hsm->timer.mask &= ~mask;
}


/**
 * Find ready hsm timers.
 *
 * Max size of the counter (32bits) is 4294967295.
 * Every ~50 days counter will be overrun.
 *
 */
unsigned short hsm_timer_check(struct hsm *hsm, unsigned int *time_lapse)
{
    unsigned int ticks;

    if (hsm->timer.mask & HSM_TIMER_1MS) {
        ticks = 1;
    } else if (hsm->timer.mask & HSM_TIMER_10MS) {
        ticks = 10 - hsm->timer.counter%10;
    } else if (hsm->timer.mask & HSM_TIMER_100MS) {
        ticks = 100 - hsm->timer.counter%100;
    } else {
        ticks = 1000 - hsm->timer.counter%1000;
    }

    ticks = (*time_lapse >= ticks) ? ticks : *time_lapse;
    *time_lapse = *time_lapse - ticks;

    hsm->timer.counter += ticks;

    unsigned short ready_mask = HSM_TIMER_DISABLED;

    if (hsm->timer.mask & HSM_TIMER_1MS)
        ready_mask |= HSM_TIMER_1MS;
    if ((hsm->timer.mask & HSM_TIMER_10MS) && (hsm->timer.counter%10 == 0))
        ready_mask |= HSM_TIMER_10MS;
    if ((hsm->timer.mask & HSM_TIMER_100MS) && (hsm->timer.counter%100 == 0))
        ready_mask |= HSM_TIMER_100MS;
    if ((hsm->timer.mask & HSM_TIMER_1S) && (hsm->timer.counter%1000 == 0))
        ready_mask |= HSM_TIMER_1S;
    if ((hsm->timer.mask & HSM_TIMER_10S) && (hsm->timer.counter%10000 == 0))
        ready_mask |= HSM_TIMER_10S;
    if ((hsm->timer.mask & HSM_TIMER_1M) && (hsm->timer.counter%60000 == 0))
        ready_mask |= HSM_TIMER_1M;

    return ready_mask;
}


/**
 * Return hsm timer counter in milliseconds since state was entered.
 *
 */
unsigned long hsm_timer_milis(struct hsm *hsm)
{
    return hsm->timer.counter * 1;
}


/**
 * Return hsm timer counter in seconds since state was entered.
 *
 */
unsigned long hsm_timer_seconds(struct hsm *hsm)
{
    return hsm->timer.counter / 1000;
}




/**
 * Initializer.
 *
 */
void hsm_init(struct hsm *hsm, int initial_state, const struct hsm_state *state_router)
{
    hsm->state = initial_state;
    hsm->current_state = initial_state;
    hsm->unwind = 0;
    hsm->router = state_router;
}


/**
 * Cleaner.
 *
 */
void hsm_clean(struct hsm *hsm)
{
    (void)hsm;

}


/**
 * Return current state
 */
int hsm_get_current_state(struct hsm *hsm)
{
    return hsm->current_state;
}


/**
 * Distribute event within state machine.
 *
 */
void hsm_handle_event(struct hsm *hsm, void *container, int ev, const void *data)
{
    int ret;
    hsm->current_state = hsm->state;

    do {
        // Send the event to the current state handler
        // If HSM_RET_SUPER is returned, the same event will be sent to the parent state
        int tmp_state = hsm->state;
        ret = hsm->router[tmp_state].handler(container, ev, data);
    } while (ret == HSM_RET_SUPER);

    if (ret == HSM_RET_TRANS) {
        if (hsm->current_state != hsm->state) {
            // Change HSM state
            int new_state = hsm->state; // Save new state
            hsm->state = hsm->current_state;
            // Continuously send EXIT event while state is unwound.
            while (hsm->unwind > 0) {
                int tmp_state = hsm->state;
#if DEBUG_HSM
                TRACE_INFO("hsm: Exit  %s\n", hsm->router[tmp_state].name);
#endif
                ret = hsm->router[tmp_state].handler(container, HSM_EV_EXIT_STATE, 0);
                hsm->unwind = (ret == HSM_RET_SUPER) ? hsm->unwind-1 : 0;
            }
            hsm->state = new_state;    // Restore new state
            // Continuously send ENTER event while state is changed.
            do {
                hsm->current_state = hsm->state;
                hsm_timer_init(hsm);
                hsm_trace_state(hsm->state);
#if DEBUG_HSM
                TRACE_INFO("hsm: Enter %s\n", hsm->router[hsm->current_state].name);
#endif
                ret = hsm->router[hsm->current_state].handler(container, HSM_EV_ENTER_STATE, 0);
            } while ((ret == HSM_RET_TRANS) && (hsm->current_state != hsm->state));
        }
    }

    // Save final state. There are two possibilities:
    // 1.State hasn't been changed, restore current state
    // 2.State changed, save last state that returned value other than HSM_RET_TRANS
    hsm->state = hsm->current_state;
}


/**
 * Handle state machine timers.
 *
 * The 'time_lapse' defines number of milliseconds since last call
 *
 */
void hsm_handle_time(struct hsm *hsm, void *container, unsigned int time_lapse)
{
    while (time_lapse > 0) {
        // Check timers until time laps is exhausted
        unsigned short ready_mask = hsm_timer_check(hsm, &time_lapse);

        if (ready_mask & HSM_TIMER_1MS)
            hsm_handle_event(hsm, container, HSM_EV_TIMER_1MS, 0);

        if (ready_mask & HSM_TIMER_10MS)
            hsm_handle_event(hsm, container, HSM_EV_TIMER_10MS, 0);

        if (ready_mask & HSM_TIMER_100MS)
            hsm_handle_event(hsm, container, HSM_EV_TIMER_100MS, 0);

        if (ready_mask & HSM_TIMER_1S)
            hsm_handle_event(hsm, container, HSM_EV_TIMER_1S, 0);

        if (ready_mask & HSM_TIMER_10S)
            hsm_handle_event(hsm, container, HSM_EV_TIMER_10S, 0);

        if (ready_mask & HSM_TIMER_1M)
            hsm_handle_event(hsm, container, HSM_EV_TIMER_1M, 0);
    }
}

