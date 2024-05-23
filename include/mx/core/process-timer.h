
#ifndef __MX_PROCESS_TIMER_H_
#define __MX_PROCESS_TIMER_H_


#include "mx/core/message.h"

#include "mx/timer.h"



struct process;

struct process_timer
{
    struct process_timer *next;
    struct process *process;
    struct timer timer;
    msgtype_t event;
};

void process_timer_start(struct process_timer *self, struct process *proc, uint32_t time_ms, msgtype_t ev);
void process_timer_stop(struct process_timer *self);
bool process_timer_running(struct process_timer *self);
void process_timer_handler(void);



#endif /* __MX_PROCESS_TIMER_H_ */
