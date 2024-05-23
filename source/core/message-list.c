
#include "mx/core/message-list.h"
#include "mx/cba.h"





/**
 * Allocate message pointer object
 *
 */

struct msg_ptr* msg_ptr_malloc(struct cba *cba, size_t msg_size)
{
    uint16_t malloc_size = sizeof(struct msg_ptr) - sizeof(struct msg) + msg_size;
    struct msg_ptr *msg_ptr = cba_malloc(cba, malloc_size);
    if (msg_ptr == NULL)
        return NULL;

    msg_ptr->next = NULL;
    msg_ptr->private = NULL;
    msg_ptr->length = msg_size;
    return msg_ptr;
}


/**
 * Free message pointer object
 *
 */
struct msg_ptr* msg_ptr_free(struct cba *cba, struct msg_ptr *msg_ptr)
{
    return cba_free(cba, msg_ptr);
}







/**
 * Initialize message list
 *
 */
void msg_list_init(struct msg_list *self)
{
    self->msg_head = NULL;
    self->msg_tail = NULL;
    self->length = 0;
}


/**
 * Push message object
 *
 * The 'msg' parameter has to be allocated via msg_malloc().
 *
 */
struct msg_ptr* msg_list_push(struct msg_list *self, struct msg_ptr *msg_ptr)
{
    msg_ptr->next = NULL;

    if (self->msg_tail)
        self->msg_tail->next = msg_ptr;
    self->msg_tail = msg_ptr;

    if (self->msg_head == NULL)
        self->msg_head = msg_ptr;

    self->length++;

    return msg_ptr;
}


/**
 * Pop message object
 *
 */
struct msg_ptr* msg_list_pop(struct msg_list *self)
{
    struct msg_ptr *msg_ptr = self->msg_head;
    if (msg_ptr == NULL)
        return NULL;

    self->msg_head = msg_ptr->next;

    if (self->msg_tail == msg_ptr)
        self->msg_tail = NULL;

    if (self->length > 0)
        self->length--;

    return msg_ptr;
}


/**
 * Peek message object
 *
 */
struct msg_ptr* msg_list_peek(struct msg_list *self)
{
    return self->msg_head;
}


/**
 * Remove message object from message list
 *
 */
struct msg_ptr* msg_list_remove(struct msg_list *self, struct msg_ptr *msg_ptr)
{
    struct msg_ptr *ptr = self->msg_head;
    struct msg_ptr **tmp = &self->msg_head;

    while (ptr) {
        if (ptr == msg_ptr) {
            *tmp = ptr->next;
            if (self->length > 0)
                self->length--;
            return ptr;
        }
        tmp = &ptr->next;
        ptr = ptr->next;
    }

    return NULL;
}


/**
 * Find message after message type
 *
 */
struct msg_ptr* msg_list_find_msgtype(struct msg_list *self, msgtype_t type)
{
    struct msg_ptr *msg_ptr = self->msg_head;
    while (msg_ptr) {
        if (msg_ptr->msg.type == type)
            return msg_ptr;

        msg_ptr = msg_ptr->next;
    }

    return NULL;
}



/**
 * Return number of listed messages
 *
 */
size_t msg_list_length(struct msg_list *self)
{
    return self->length;
}
