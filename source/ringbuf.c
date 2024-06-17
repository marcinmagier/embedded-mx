
#include "mx/ringbuf.h"





#define ACCESS_ONCE_DATA(x) (*(volatile uint8_t *)&(x))
#define ACCESS_ONCE_IDX(x) (*(volatile ringbuf_len_t *)&(x))



void ringbuf_init(struct ringbuf *self, uint8_t *buffer, ringbuf_len_t size)
{
    self->data = buffer;
    self->mask = size - 1;
    self->put_idx = 0;
    self->get_idx = 0;
}


bool ringbuf_put(struct ringbuf *self, uint8_t c)
{
    if (ringbuf_is_full(self))
        return false;

    ACCESS_ONCE_DATA(self->data[self->put_idx]) = c;
    ACCESS_ONCE_IDX(self->put_idx) = (self->put_idx + 1) & self->mask;
    return true;
}


bool ringbuf_get(struct ringbuf *self, uint8_t *c)
{
    if (!ringbuf_is_empty(self)) {
        *c = ACCESS_ONCE_DATA(self->data[self->get_idx]);
        ACCESS_ONCE_IDX(self->get_idx) = (self->get_idx + 1) & self->mask;
        return true;
    }

    return false;
}


ringbuf_len_t ringbuf_size(struct ringbuf *self)
{
    return self->mask + 1;
}


ringbuf_len_t ringbuf_elements(struct ringbuf *self)
{
    return (self->put_idx - self->get_idx) & self->mask;
}


bool ringbuf_is_empty(struct ringbuf *self)
{
    return (ringbuf_elements(self) == 0);
}


bool ringbuf_is_full(struct ringbuf *self)
{
    return (ringbuf_elements(self) == self->mask);
}
