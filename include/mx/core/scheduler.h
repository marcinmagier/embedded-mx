
#ifndef __MX_SCHEDULER_H_
#define __MX_SCHEDULER_H_


#include "mx/core/message.h"
#include "mx/core/message-list.h"

#include "mx/cba.h"


struct process;
struct process_timer;

struct scheduler
{
    struct process *process_head;
    struct process *process_current;

    struct process_timer *process_timer_head;

    struct msg_list messages;
    struct cba cba;

    unsigned int poll_requested;

};




void scheduler_init(struct scheduler *self, void *buffer, uint16_t len);
void scheduler_start_process(struct scheduler *self, struct process *proc);
void scheduler_exit_process(struct scheduler *self, struct process *proc);

unsigned int scheduler_run(struct scheduler *self);
unsigned int scheduler_events(struct scheduler *self);

struct process* scheduler_get_current_process(struct scheduler *self);

struct msg* scheduler_malloc(struct scheduler *self, uint32_t size);
struct msg* scheduler_free(struct scheduler *self, struct msg *msg);

void scheduler_post_msg(struct scheduler *self, struct process *proc, struct msg *msg);
void scheduler_handle_msg(struct scheduler *self, struct process *proc, struct msg *msg);

void scheduler_timer_start(struct scheduler *self, struct process_timer *timer, uint32_t time_ms);
void scheduler_timer_stop(struct scheduler *self, struct process_timer *timer);
void scheduler_timer_handler(struct scheduler *self);


#endif /* __MX_SCHEDULER_H_ */
