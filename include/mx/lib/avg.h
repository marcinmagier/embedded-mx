
#ifndef __MX_MISC_AVG_H_
#define __MX_MISC_AVG_H_


#include <stdint.h>
#include <stdbool.h>

typedef uint8_t     extreme_size_t;


struct avg
{
    uint32_t sum;
    uint32_t elements;

    uint32_t *min;
    uint32_t *max;
    extreme_size_t min_size;
    extreme_size_t min_len;
    extreme_size_t max_size;
    extreme_size_t max_len;
};


void avg_init(struct avg *self, uint32_t *min, extreme_size_t min_size, uint32_t *max, extreme_size_t max_size);
void avg_add(struct avg *self, uint32_t value);
bool avg_ready(struct avg *self);

uint32_t avg_calculate(struct avg *self);


#endif /* __MX_MISC_AVG_H_ */
