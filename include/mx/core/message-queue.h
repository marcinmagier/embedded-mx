
#ifndef __MX_MESSAGE_QUEUE_H_
#define __MX_MESSAGE_QUEUE_H_


#include "mx/core/message.h"
#include "mx/core/message-list.h"

#include <stdint.h>
#include <stddef.h>





enum msq_queue_priority_e
{
    MSG_PRIO_HIGH = 0,
    MSG_PRIO_NORMAL,
    MSG_PRIO_LOW,

    MSG_PRIO_LENGTH,      // For internal use only

    MSG_PRIO_ANY,       // May be used to check if message is pending
};





struct msg_queue
{
    struct msg_list list_prio[MSG_PRIO_LENGTH];
};





void msg_queue_init(struct msg_queue *self);

struct msg_list* msg_queue_get_msg_list(struct msg_queue *self, uint8_t prio);

struct msg_ptr* msg_queue_push(struct msg_queue *self, uint8_t prio, struct msg_ptr *msg);
struct msg_ptr* msg_queue_pop(struct msg_queue *self, uint8_t *prio);
struct msg_ptr* msg_queue_peek(struct msg_queue *self, uint8_t *prio);

struct msg_ptr* msg_queue_find_msgtype(struct msg_queue *self, uint8_t *prio, msgtype_t type);

size_t msg_queue_length(struct msg_queue *self);



#endif /* __MX_MESSAGE_QUEUE_H_ */
