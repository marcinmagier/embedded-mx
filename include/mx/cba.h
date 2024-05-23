
#ifndef __MX_CBA_H_
#define __MX_CBA_H_

#include <stdint.h>


struct cba
{
    uint8_t *buffer;
    uint16_t size;

    uint16_t free_idx;
    uint16_t head_idx;
    uint16_t tail_idx;
};



void cba_init(struct cba *cba, void *buffer, uint16_t len);
void cba_clean(struct cba *cba);
void cba_reset(struct cba *cba);


void* cba_malloc(struct cba *cba, uint16_t len);
#ifdef CBA_REALLOC
void* cba_realloc(struct cba *cba, void *chunk, uint16_t len);
#endif
void* cba_free(struct cba *cba, void *chunk);


#ifdef CBA_PRINT
void cba_print(struct cba *cba);
#endif


#ifdef CBA_VALIDATION
extern void cba_report_exception(const char *msg);
#endif

#endif /* __MX_CBA_H_ */
