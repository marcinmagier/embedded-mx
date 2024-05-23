
#ifndef __MX_RINGBUF_H_
#define __MX_RINGBUF_H_


#include <stdint.h>
#include <stdbool.h>



struct ringbuf {
  volatile uint8_t *data;
  uint8_t mask;

  volatile uint8_t put_ptr;
  volatile uint8_t get_ptr;
};

void    ringbuf_init(struct ringbuf *self, uint8_t *buffer, uint8_t size);
bool    ringbuf_put(struct ringbuf *self, uint8_t c);
bool    ringbuf_get(struct ringbuf *self, uint8_t *c);
uint8_t ringbuf_size(struct ringbuf *self);
uint8_t ringbuf_elements(struct ringbuf *self);


#endif /* __MX_RINGBUF_H_ */
