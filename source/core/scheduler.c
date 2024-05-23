
#include "mx/core/scheduler.h"
#include "mx/core/process.h"
#include "mx/core/process-timer.h"

#ifdef DEBUG_PROCESS
  #include "mx/trace.h"
#endif





struct scheduler default_scheduler;





static void call_process(struct process *proc, msgtype_t ev, struct msg *msg);


static void exit_process(struct process *proc, struct process *fromprocess)
{
    register struct process *tmp;
    struct scheduler *sched = proc->sched;
    struct process *old_current = sched->process_current;

#if (DEBUG_PROCESS >= 1)
    TRACE_INFO("sched: Exit '%s'", PROCESS_NAME_STRING(proc));
#endif

    /* Make sure the process is in the process list before we try to exit it. */
    for (tmp = sched->process_head; tmp != proc && tmp != NULL; tmp = tmp->next)
        ;
    if (tmp == NULL)
        return;

    if (process_is_running(proc)) {
        /* Process was running */
        proc->state = PROCESS_STATE_NONE;

        /*
         * Post a synchronous event to all processes to inform them that
         * this process is about to exit. This will allow services to
         * deallocate state associated with this process.
         */
        for (tmp = sched->process_head; tmp != NULL; tmp = tmp->next) {
            if (proc != tmp) {
                struct mx_msg_process msg_proc = {PROCESS_EV_FINISHED, proc};
                call_process(tmp, msg_proc.type, (struct msg*)&msg_proc);
            }
        }

        if (proc->thread != NULL && proc != fromprocess) {
            /* Post the exit event to the process that is about to exit. */
            sched->process_current = proc;
            proc->thread(proc, PROCESS_EV_EXIT, NULL);
        }
    }

    if (proc == sched->process_head) {
        sched->process_head = sched->process_head->next;
    } else {
        for (tmp = sched->process_head; tmp != NULL; tmp = tmp->next) {
            if (tmp->next == proc) {
                tmp->next = proc->next;
                break;
            }
        }
    }

    sched->process_current = old_current;
}


void call_process(struct process *proc, msgtype_t ev, struct msg *msg)
{
    int ret;

#if (DEBUG_PROCESS >= 2)
    if(proc->state == PROCESS_STATE_CALLED) {
        TRACE_DEBUG("sched: Process '%s' called again with event %02X", PROCESS_NAME_STRING(proc), ev);
    }
#endif

    if ((proc->state & PROCESS_STATE_RUNNING) && proc->thread != NULL) {
#if (DEBUG_PROCESS >= 2)
        switch (ev) {
            case PROCESS_EV_TIMER:
            case PROCESS_EV_POLL:
                break;
            default:
                TRACE_DEBUG("sched: Calling process '%s' with event %02X", PROCESS_NAME_STRING(proc), ev);
        }
#endif
        struct process *caller = proc->sched->process_current;
        proc->sched->process_current = proc;
        proc->state = PROCESS_STATE_CALLED;
        ret = proc->thread(proc, ev, msg);
        proc->sched->process_current = caller;
        if (ret == PT_EXITED || ret == PT_ENDED || ev == PROCESS_EV_EXIT) {
            exit_process(proc, proc);
        } else {
            proc->state = PROCESS_STATE_RUNNING;
        }
    }
}







static void scheduler_utilize_poll(struct scheduler *self);
static void scheduler_utilize_message(struct scheduler *self);


/**
 * Initialize scheduler
 *
 */
void scheduler_init(struct scheduler *self, void *buffer, uint16_t len)
{
    cba_init(&self->cba, buffer, len);
    msg_list_init(&self->messages);
    self->process_head = NULL;
    self->process_current = NULL;
    self->poll_requested = 0;
    self->process_timer_head = NULL;
}


/**
 * Start process
 *
 */
void scheduler_start_process(struct scheduler *self, struct process *proc)
{
    /* First make sure that we don't try to start a process that is already running. */
    struct process *tmp;
    for (tmp = self->process_head; tmp != proc && tmp != NULL; tmp = tmp->next)
        ;
    /* If we found the process on the process list, we bail out. */
    if (tmp == proc)
        return;

    /* Put on the procs list.*/
    PT_INIT(&proc->pt);
    proc->state = PROCESS_STATE_RUNNING;
    proc->sched = self;
    proc->next = self->process_head;
    self->process_head = proc;

#if (DEBUG_PROCESS >= 1)
    TRACE_INFO("sched: Starting '%s'", PROCESS_NAME_STRING(proc));
#endif

    /* Post a synchronous initialization event to the process. */
    call_process(proc, PROCESS_EV_INIT, NULL);

}


/**
 * Exit process
 *
 */
void scheduler_exit_process(struct scheduler *self, struct process *proc)
{
    exit_process(proc, scheduler_get_current_process(self));
}


/**
 * Run scheduler single iteration
 *
 */
unsigned int scheduler_run(struct scheduler *self)
{
    /* Process poll events. */
    if (self->poll_requested) {
        scheduler_utilize_poll(self);
    }

    /* Process one message from the queue */
    scheduler_utilize_message(self);

    return scheduler_events(self);
}


/**
 * Return number of waiting events
 *
 */
unsigned int scheduler_events(struct scheduler *self)
{
    return msg_list_length(&self->messages) + self->poll_requested;
}


/**
 * Return current proccess
 *
 */
struct process* scheduler_get_current_process(struct scheduler *self)
{
    return self->process_current;
}


/**
 * Handle poll
 *
 */
void scheduler_utilize_poll(struct scheduler *self)
{
    self->poll_requested = 0;

    /* Call the processes that needs to be polled. */
    struct process *tmp;
    for (tmp = self->process_head; tmp != NULL; tmp = tmp->next) {
        if (tmp->needspoll) {
            tmp->state = PROCESS_STATE_RUNNING;
            tmp->needspoll = 0;
            call_process(tmp, PROCESS_EV_POLL, NULL);
        }
    }
}


/**
 * Handle single message
 *
 */
void scheduler_utilize_message(struct scheduler *self)
{
    struct msg_ptr *msg_ptr = msg_list_pop(&self->messages);
    if (msg_ptr == NULL)
        return;

    struct msg *msg = &msg_ptr->msg;
    struct process *receiver = (struct process*)msg_ptr->private;

    if (receiver == PROCESS_BROADCAST) {
        struct process *p;
        for (p = self->process_head; p != NULL; p = p->next) {
            /* If we have been requested to poll a process, we do this in between processing the broadcast event. */
            if (self->poll_requested)
                scheduler_utilize_poll(self);
            call_process(p, msg->type, msg);
        }
    }
    else {
        /* This is not a broadcast event, so we deliver it to the specified process. */
        /* If the event was an INIT event, we should also update the state of the process. */
        if (msg->type == PROCESS_EV_INIT)
            receiver->state = PROCESS_STATE_RUNNING;

        call_process(receiver, msg->type, msg);
    }

    msg_ptr_free(&self->cba, msg_ptr);
}


/**
 * Allocate message object
 *
 */
struct msg* scheduler_malloc(struct scheduler *self, uint32_t size)
{
    struct msg_ptr *ptr = msg_ptr_malloc(&self->cba, size);
    if (ptr)
        return &ptr->msg;

    return NULL;
}


/**
 * Free message object
 *
 */
struct msg* scheduler_free(struct scheduler *self, struct msg *msg)
{
    struct msg_ptr *ptr = cast_msg_ptr(msg);
    msg_ptr_free(&self->cba, ptr);
    return NULL;
}


/**
 * Post message
 *
 */
void scheduler_post_msg(struct scheduler *self, struct process *proc, struct msg *msg)
{
#if (DEBUG_PROCESS >= 2)
    TRACE_DEBUG("sched: Process '%s' posts event %02X to process '%s', waiting events %lu",
                    self->process_current == NULL ? "<sys>" : PROCESS_NAME_STRING(self->process_current),
                    msg->type,
                    proc == PROCESS_BROADCAST ? "<broadcast>" : PROCESS_NAME_STRING(proc),
                    (unsigned long)msg_list_length(&self->messages));
#endif

    struct msg_ptr *msg_ptr = cast_msg_ptr(msg);
    msg_ptr->private = proc;
    msg_list_push(&self->messages, msg_ptr);
}


/**
 * Handle message directly
 *
 */
void scheduler_handle_msg(struct scheduler *self, struct process *proc, struct msg *msg)
{
    if (proc == PROCESS_BROADCAST) {
        struct process *p;
        for (p = self->process_head; p != NULL; p = p->next)
            call_process(p, msg->type, msg);
    }
    else {
        call_process(proc, msg->type, msg);
    }
}



void scheduler_timer_start(struct scheduler *self, struct process_timer *proctimer, uint32_t time_ms)
{
    timer_start(&proctimer->timer, TIMER_MS, time_ms);

    struct process_timer *tmp;
    for (tmp = self->process_timer_head; tmp != NULL && tmp != proctimer; tmp = tmp->next) { }
    if (tmp == proctimer)
        return;     // Timer already in the list

    proctimer->next = self->process_timer_head;
    self->process_timer_head = proctimer;
}


void scheduler_timer_stop(struct scheduler *self, struct process_timer *proctimer)
{
    timer_stop(&proctimer->timer);

    if (proctimer == self->process_timer_head) {
        self->process_timer_head = self->process_timer_head->next;
    } else {
        struct process_timer *tmp;
        for (tmp = self->process_timer_head; tmp != NULL; tmp = tmp->next) {
            if (tmp->next == proctimer) {
                tmp->next = proctimer->next;
                break;
            }
        }
    }
}


static struct process_timer* scheduler_timer_find_expired(struct scheduler *self)
{
    struct process_timer *tmp, *prev = self->process_timer_head;
    for (tmp = prev; tmp != NULL; prev = tmp, tmp = tmp->next) {
        if (timer_expired(&tmp->timer)) {
            timer_stop(&tmp->timer);
            if (tmp == self->process_timer_head) {
               self->process_timer_head = tmp->next;
            } else {
                prev->next = tmp->next;
            }
            return tmp;
        }
    }

    return NULL;
}


void scheduler_timer_handler(struct scheduler *self)
{
    struct process_timer *tmp;
    while (tmp = scheduler_timer_find_expired(self), tmp) {
        struct msg msg;
        msg.type = tmp->event;
        scheduler_handle_msg(self, tmp->process, &msg);
    }
}
