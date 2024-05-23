
#include "mx/lock.h"





/**
 * Initializer
 *
 */
void locker_init(struct locker *self)
{
    self->locked = false;
    timer_stop(&self->timer);
}


/**
 * Cleaner
 *
 */
void locker_clean(struct locker *self)
{
    self->locked = false;
}


/**
 * Restart locking timer
 *
 */
void locker_restart(struct locker *self)
{
    timer_restart(&self->timer);
}


/**
 * Lock locker
 *
 */
int locker_lock(struct locker *self, uint8_t flags, uint32_t time)
{
    int status = LOCK_SUCCESS;

    if (self->locked) {
        if (!timer_expired(&self->timer))
            return LOCK_FAIL;

        // Lock is obsolete
        status = LOCK_INTERCEPTED;
    }

    timer_start(&self->timer, flags, time);
    self->locked = true;

    return status;
}


/**
 * Release locker
 *
 */
void locker_release(struct locker *self)
{
    self->locked = false;
}
