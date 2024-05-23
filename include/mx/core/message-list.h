
#ifndef __MX_MESSAGE_LIST_H_
#define __MX_MESSAGE_LIST_H_



#include "mx/core/message.h"

#include <stdint.h>
#include <stddef.h>



struct msg_ptr
{
    struct msg_ptr *next;
    void *private;
    size_t length;
    struct msg msg;
};


#define cast_msg_ptr(ptr) (struct msg_ptr*)( (char *)ptr - offsetof(struct msg_ptr, msg) )



struct cba;

struct msg_ptr* msg_ptr_malloc(struct cba *cba, size_t msg_size);
struct msg_ptr* msg_ptr_free(struct cba *cba, struct msg_ptr *msg_ptr);







struct msg_list
{
    struct msg_ptr *msg_head;
    struct msg_ptr *msg_tail;

    size_t length;
};


void msg_list_init(struct msg_list *self);

struct msg_ptr* msg_list_push(struct msg_list *self, struct msg_ptr *msg_ptr);
struct msg_ptr* msg_list_pop(struct msg_list *self);
struct msg_ptr* msg_list_peek(struct msg_list *self);
struct msg_ptr* msg_list_remove(struct msg_list *self, struct msg_ptr *msg_ptr);

struct msg_ptr* msg_list_find_msgtype(struct msg_list *self, msgtype_t type);

size_t msg_list_length(struct msg_list *self);





#endif /* __MX_MESSAGE_LIST_H_ */
