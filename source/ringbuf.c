
#include "mx/ringbuf.h"





#define ACCESS_ONCE_U8(x) (*(volatile uint8_t *)&(x))



void ringbuf_init(struct ringbuf *self, uint8_t *buffer, uint8_t size)
{
  self->data = buffer;
  self->mask = size - 1;
  self->put_ptr = 0;
  self->get_ptr = 0;
}


bool ringbuf_put(struct ringbuf *self, uint8_t c)
{
    if(((self->put_ptr - self->get_ptr) & self->mask) == self->mask)
      return false;

    ACCESS_ONCE_U8(self->data[self->put_ptr]) = c;
    ACCESS_ONCE_U8(self->put_ptr) = (self->put_ptr + 1) & self->mask;
    return true;
}

bool ringbuf_get(struct ringbuf *self, uint8_t *c)
{
    if(((self->put_ptr - self->get_ptr) & self->mask) > 0) {
        *c = ACCESS_ONCE_U8(self->data[self->get_ptr]);
        ACCESS_ONCE_U8(self->get_ptr) = (self->get_ptr + 1) & self->mask;
        return true;
    }

    return false;
}


uint8_t ringbuf_size(struct ringbuf *self)
{
    return self->mask + 1;
}


uint8_t ringbuf_elements(struct ringbuf *self)
{
  return (self->put_ptr - self->get_ptr) & self->mask;
}
