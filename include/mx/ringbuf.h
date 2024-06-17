
#ifndef __MX_RINGBUF_H_
#define __MX_RINGBUF_H_


#include <stdint.h>
#include <stdbool.h>



#ifndef RINGBUF_LEN_SIZE
  #define RINGBUF_LEN_SIZE      1
#endif

#if RINGBUF_LEN_SIZE == 1
  typedef uint8_t   ringbuf_len_t;
#elif RINGBUF_LEN_SIZE == 2
  typedef uint16_t  ringbuf_len_t;
#else
  typedef uint32_t  ringbuf_len_t;
#endif





struct ringbuf {
  volatile uint8_t *data;
  ringbuf_len_t mask;

  volatile ringbuf_len_t put_idx;
  volatile ringbuf_len_t get_idx;
};



void ringbuf_init(struct ringbuf *self, uint8_t *buffer, ringbuf_len_t size);
bool ringbuf_put(struct ringbuf *self, uint8_t c);
bool ringbuf_get(struct ringbuf *self, uint8_t *c);

ringbuf_len_t ringbuf_size(struct ringbuf *self);
ringbuf_len_t ringbuf_elements(struct ringbuf *self);

bool ringbuf_is_empty(struct ringbuf *self);
bool ringbuf_is_full(struct ringbuf *self);


#endif /* __MX_RINGBUF_H_ */
