
#ifndef __MX_LOCK_H_
#define __MX_LOCK_H_


#include "mx/timer.h"

#include <stdbool.h>



enum locker_status_e
{
    LOCK_FAIL = 0,
    LOCK_SUCCESS = 1,
    LOCK_INTERCEPTED = 2,
};



struct locker
{
    struct timer timer;
    bool locked;
};




void locker_init(struct locker *self);
void locker_clean(struct locker *self);
void locker_restart(struct locker *self);

int  locker_lock(struct locker *self, uint8_t flags, uint32_t time);
void locker_release(struct locker *self);


static inline bool locker_is_locked(struct locker *self)
{
    return self->locked;
}

static inline bool locker_is_expired(struct locker *self)
{
    return self->locked ? timer_expired(&self->timer) : false;
}


#endif /* __MX_LOCK_H_ */
