
#include "mx/core/message-queue.h"







/**
 * Initialize message queue
 *
 */
void msg_queue_init(struct msg_queue *self)
{
    for (unsigned int i=0; i<MSG_PRIO_LENGTH; i++)
        msg_list_init(&self->list_prio[i]);
}


/**
 * Return message list of given priority
 *
 * In case prio argument is MSG_PRIO_ANY then first non-empty list is returned.
 *
 */
struct msg_list* msg_queue_get_msg_list(struct msg_queue *self, uint8_t prio)
{
    if (prio < MSG_PRIO_LENGTH)
        return &self->list_prio[prio];

    if (prio == MSG_PRIO_ANY) {
        for (unsigned int i=0; i<MSG_PRIO_LENGTH; i++) {
            struct msg_list *list = &self->list_prio[i];
            if (msg_list_peek(list))
                return list;
        }
    }

    return NULL;;
}


/**
 * Push message object
 *
 * The 'msg_ptr' parameter has to be allocated via msg_malloc().
 *
 */
struct msg_ptr* msg_queue_push(struct msg_queue *self, uint8_t prio, struct msg_ptr *msg_ptr)
{
    if (prio >= MSG_PRIO_LENGTH)
        return NULL;

    return msg_list_push(&self->list_prio[prio], msg_ptr);
}


/**
 * Pop message object
 *
 */
struct msg_ptr* msg_queue_pop(struct msg_queue *self, uint8_t *prio)
{
    for (unsigned int i=0; i<MSG_PRIO_LENGTH; i++) {
        struct msg_ptr *msg_ptr = msg_list_pop(&self->list_prio[i]);
        if (msg_ptr) {
            if (prio)
                *prio = i;
            return msg_ptr;
        }
    }

    return NULL;
}


/**
 * Peek message object
 *
 */
struct msg_ptr* msg_queue_peek(struct msg_queue *self, uint8_t *prio)
{
    for (unsigned int i=0; i<MSG_PRIO_LENGTH; i++) {
        struct msg_ptr *msg_ptr = msg_list_peek(&self->list_prio[i]);
        if (msg_ptr) {
            if (prio)
                *prio = i;
            return msg_ptr;
        }
    }

    return NULL;
}


struct msg_ptr* msg_queue_find_msgtype(struct msg_queue *self, uint8_t *prio, msgtype_t type)
{
    int tmp_prio = MSG_PRIO_ANY;
    if (prio)
        tmp_prio = *prio;

    if (tmp_prio < MSG_PRIO_LENGTH)
        return msg_list_find_msgtype(&self->list_prio[tmp_prio], type);

    for (unsigned int i=0; i<MSG_PRIO_LENGTH; i++) {
        struct msg_ptr *msg_ptr = msg_list_find_msgtype(&self->list_prio[i], type);
        if (msg_ptr) {
            if (prio)
                *prio = i;
            return msg_ptr;
        }
    }

    return NULL;
}


/**
 * Return number of queued messages
 *
 */
size_t msg_queue_length(struct msg_queue *self)
{
    size_t length = 0;
    for (unsigned int i=0; i<MSG_PRIO_LENGTH; i++)
        length += msg_list_length(&self->list_prio[i]);

    return length;
}


