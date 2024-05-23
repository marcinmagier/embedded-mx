
#include "mx/core/process.h"
#include "mx/core/process-timer.h"
#include "mx/core/scheduler.h"

#include <string.h>




extern struct scheduler default_scheduler;



/**
 * Initialize process
 *
 */
void process_init(void *buffer, uint16_t len)
{
    scheduler_init(&default_scheduler, buffer, len);
}


/**
 * Start process
 *
 */
void process_start(struct process *self)
{
    scheduler_start_process(&default_scheduler, self);
}


/**
 * Exit process
 *
 */
void process_exit(struct process *self)
{
    scheduler_exit_process(&default_scheduler, self);
}


/**
 * Poll process
 *
 */
void process_poll(struct process *self)
{
    if (self != NULL) {
        if (self->state == PROCESS_STATE_RUNNING || self->state == PROCESS_STATE_CALLED) {
            self->needspoll = 1;
            self->sched->poll_requested = 1;
        }
    }
}


/**
 * Run single process iteration
 *
 */
unsigned int process_run(void)
{
    return scheduler_run(&default_scheduler);
}


/**
 * Returns number of waiting events
 *
 */
unsigned int process_events(void)
{
    return scheduler_events(&default_scheduler);
}


/**
 * Checks if process is running
 *
 */
int process_is_running(struct process *self)
{
    return self->state != PROCESS_STATE_NONE;
}


/**
 * Returns current process
 *
 */
struct process* process_get_current(void)
{
    return scheduler_get_current_process(&default_scheduler);
}



/**
 * Allocate message object
 *
 */
struct msg* process_malloc(uint32_t size)
{
    return scheduler_malloc(&default_scheduler, size);
}


/**
 * Free message object
 */
struct msg* process_free(struct msg *msg)
{
    return scheduler_free(&default_scheduler, msg);
}


/**
 * Post message
 *
 * Message object has to be allocated with process_malloc()
 *
 */
int process_post_msg(struct process *self, struct msg *msg)
{
    scheduler_post_msg(&default_scheduler, self, msg);
    return PROCESS_SUCCESS;
}


/**
 * Directly handle message
 *
 * Current process is continued after processing message.
 *
 */
void process_handle_msg(struct process *self, struct msg *msg)
{
    scheduler_handle_msg(&default_scheduler, self, msg);
}


/**
 * Directly handle message containing 0 parameters
 *
 */
void process_handle_msg_p0(struct process *self, msgtype_t type)
{
    struct msg msg = {
        .type = type
    };

    scheduler_handle_msg(&default_scheduler, self, &msg);
}


/**
 * Send message
 *
 * Message object will be created internally
 *
 */
int process_send_msg(struct process *self, struct msg *msg, uint32_t msg_len)
{
    struct msg *tmp = process_malloc(msg_len);
    if (tmp == NULL)
        return PROCESS_ERR_NO_MEMORY;

    memcpy(tmp, msg, msg_len);
    return process_post_msg(self, tmp);
}


/**
 * Send message containing 0 parameters
 *
 * Message object will be created internally
 *
 */
int process_send_msg_p0(struct process *self, msgtype_t type)
{
    struct msg *msg = process_malloc(sizeof(struct msg));
    if (msg == NULL)
        return PROCESS_ERR_NO_MEMORY;

    msg->type = type;
    return process_post_msg(self, msg);
}


/**
 * Send message containing 1 parameter
 *
 * Message object will be created internally
 *
 */
int process_send_msg_p1(struct process *self, msgtype_t type, uint8_t param1)
{
    struct msg_p1 *msg = (struct msg_p1*)process_malloc(sizeof(struct msg_p1));
    if (msg == NULL)
        return PROCESS_ERR_NO_MEMORY;

    msg->type = type;
    msg->param1 = param1;
    return process_post_msg(self, (struct msg*)msg);
}


/**
 * Send message containing 2 parameters
 *
 * Message object will be created internally
 *
 */
int process_send_msg_p2(struct process *self, msgtype_t type, uint8_t param1, uint8_t param2)
{
    struct msg_p2 *msg = (struct msg_p2*)process_malloc(sizeof(struct msg_p2));
    if (msg == NULL)
        return PROCESS_ERR_NO_MEMORY;

    msg->type = type;
    msg->param1 = param1;
    msg->param2 = param2;
    return process_post_msg(self, (struct msg*)msg);
}


/**
 * Send message containing 3 parameters
 *
 * Message object will be created internally
 *
 */
int process_send_msg_p3(struct process *self, msgtype_t type, uint8_t param1, uint8_t param2, uint8_t param3)
{
    struct msg_p3 *msg = (struct msg_p3*)process_malloc(sizeof(struct msg_p3));
    if (msg == NULL)
        return PROCESS_ERR_NO_MEMORY;

    msg->type = type;
    msg->param1 = param1;
    msg->param2 = param2;
    msg->param3 = param3;
    return process_post_msg(self, (struct msg*)msg);
}


/**
 * Send message containing 4 parameters
 *
 * Message object will be created internally
 *
 */
int process_send_msg_p4(struct process *self, msgtype_t type, uint8_t param1, uint8_t param2, uint8_t param3, uint8_t param4)
{
    struct msg_p4 *msg = (struct msg_p4*)process_malloc(sizeof(struct msg_p4));
    if (msg == NULL)
        return PROCESS_ERR_NO_MEMORY;

    msg->type = type;
    msg->param1 = param1;
    msg->param2 = param2;
    msg->param3 = param3;
    msg->param4 = param4;
    return process_post_msg(self, (struct msg*)msg);
}


/**
 * Send message data
 *
 * Message object will be created internally
 *
 */
int process_send_msg_data(struct process *self, msgtype_t type, uint8_t *data, uint32_t data_len)
{
    struct msg_data *msg = (struct msg_data*)process_malloc(sizeof(struct msg_data) + data_len);
    if (msg == NULL)
        return PROCESS_ERR_NO_MEMORY;

    msg->type = type;
    memcpy(msg->data, data, data_len);
    return process_post_msg(self, (struct msg*)msg);
}

