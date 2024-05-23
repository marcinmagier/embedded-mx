

#include "mx/lib/avg.h"
#include "mx/misc.h"



static bool avg_add_min(struct avg *self, uint32_t input, uint32_t *output);
static bool avg_add_max(struct avg *self, uint32_t input, uint32_t *output);



void avg_init(struct avg *self, uint32_t *min, extreme_size_t min_size, uint32_t *max, extreme_size_t max_size)
{
    self->sum = 0;
    self->elements = 0;
    self->min = min;
    self->min_size = min_size;
    self->min_len = 0;
    self->max = max;
    self->max_size = max_size;
    self->max_len = 0;
}


void avg_add(struct avg *self, uint32_t value)
{
    if (avg_add_min(self, value, &value))
        return;         // Min extreme not full, just fill array
    if (avg_add_max(self, value, &value))
        return;         // Max extreme not full, just fill array

    self->sum += value;
    self->elements++;
}


bool avg_ready(struct avg *self)
{
    if (self->elements == 0)
        return false;

    return true;
}


uint32_t avg_calculate(struct avg *self)
{
    uint32_t ret = 0;
    if (self->elements) {
        ret = self->sum / self->elements;
        if (self->elements >= 2) {
            if ((self->sum % self->elements) >= (self->elements/2) )
                ret++;
        }
    }
    return ret;
}


bool avg_add_min(struct avg *self, uint32_t input, uint32_t *output)
{
    if (self->min == NULL) {
        *output = input;
        return false;
    }

    if (self->min_len < self->min_size) {
        // Min extreme not full, just fill array
        self->min[self->min_len++] = input;
        return true;
    }

    // Find biggest value from minima
    extreme_size_t idx = 0;
    for (unsigned int i=1; i<self->min_size; i++)
        if (self->min[idx] < self->min[i])
            idx = i;
    if (self->min[idx] > input) {
        // Replace biggest minimum with new value
        uint32_t tmp = self->min[idx];
        self->min[idx] = input;
        *output = tmp;
    }
    else {
        // Current value is bigger than minima
        *output = input;
    }

    return false;
}


bool avg_add_max(struct avg *self, uint32_t input, uint32_t *output)
{
    if (self->max == NULL) {
        *output = input;
        return false;
    }

    if (self->max_len < self->max_size) {
        // Maxima not full, just fill array
        self->max[self->max_len++] = input;
        return true;
    }

    // Find lowest value from maxima
    extreme_size_t idx = 0;
    for (unsigned int i=1; i<self->max_size; i++)
        if (self->max[idx] > self->max[i])
            idx = i;
    if (self->max[idx] < input) {
        // Replace lowest maximum with new value
        uint32_t tmp = self->max[idx];
        self->max[idx] = input;
        *output = tmp;
    }
    else {
        // Current value is bigger than maxima
        *output = input;
    }

    return false;
}
