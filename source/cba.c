
#include "mx/cba.h"


#include <stdio.h>
#include <string.h>



//          4            len1            4            len2
//      | header1 | allocated_buffer | header2 | allocated_buffer |
//      |< tail                                                   |< head
//
//      |          header          |
//         2            2
//      | FA5F | <bound:1><len:15> |


#define CBA_ALLIGNMENT          4
#define CBA_HEADER_LEN          CBA_ALLIGNMENT
#define CBA_MIN_CHUNK_LEN       3

#define CBA_HEAD_MARKER         0x5FFA
#define CBA_END_MARKER          0x00

#define CBA_STATUS_BOUND_MASK   (0x8000)
#define CBA_STATUS_LEN_MASK     (0x7FFF)





struct cba_header
{
    uint16_t marker;
    uint16_t bound:1;
    uint16_t __padding__:3;
    uint16_t chunk_len:12;
    char chunk[CBA_ALLIGNMENT];
}__attribute__((packed));




#ifdef CBA_VALIDATION
static inline void cba_validate_head_marker(struct cba_header *header)
{
    if (header->marker != CBA_HEAD_MARKER)
        cba_report_exception("invalid head marker");
}

static inline void cba_validate_end_marker(struct cba_header *header)
{
    if (header->chunk[header->chunk_len-1] != CBA_END_MARKER)
        cba_report_exception("invalid end marker");
}

#define CBA_VALIDATE_HEAD_MARKER(header)    cba_validate_head_marker(header)
#define CBA_VALIDATE_END_MARKER(header)     cba_validate_end_marker(header)
#define CBA_END_MARKER_LEN                  1

#else

#define CBA_VALIDATE_HEAD_MARKER(header)
#define CBA_VALIDATE_END_MARKER(header)
#define CBA_END_MARKER_LEN                  0

#endif // CBA_VALIDATION




/**
 * Calculate aligned chunk length.
 *
 */
static uint16_t cba_chunk_len(uint16_t len)
{
    if (len < CBA_MIN_CHUNK_LEN)
        len = CBA_MIN_CHUNK_LEN;

#ifdef CBA_VALIDATION
    return (((len/CBA_ALLIGNMENT) + 1) * CBA_ALLIGNMENT);
#else
    return (len%CBA_ALLIGNMENT == 0) ? len : (((len/CBA_ALLIGNMENT) + 1) * CBA_ALLIGNMENT);
#endif
}


/**
 * Find index of the given chunk's header.
 *
 */
static uint16_t cba_chunk_idx(struct cba *cba, void *chunk)
{
    struct cba_header *header = (struct cba_header*) ((char*)chunk - CBA_HEADER_LEN);
    CBA_VALIDATE_HEAD_MARKER(header);

    return (char*)header - (char*)cba->buffer;
}


/**
 * Find index of the next header.
 *
 */
static uint16_t cba_next_chunk_idx(struct cba *cba, uint16_t idx)
{
    struct cba_header *header = (struct cba_header*) &cba->buffer[idx];
    CBA_VALIDATE_HEAD_MARKER(header);

    return (idx + CBA_HEADER_LEN + header->chunk_len) % cba->size;
}


/**
 * Modify or create new memory chunk.
 *
 * Given index points to chunk's header.
 *
 */
static void* cba_reallocate_chunk(struct cba *cba, uint16_t idx, uint16_t len)
{
    struct cba_header *header = (struct cba_header*) &cba->buffer[idx];
    header->marker = CBA_HEAD_MARKER;
    header->bound = 1;
    header->chunk_len = len;
#ifdef CBA_VALIDATION
    header->chunk[len-1] = CBA_END_MARKER;
#endif

    cba->head_idx = idx;
    cba->free_idx = (idx + CBA_HEADER_LEN + header->chunk_len) % cba->size;

    return header->chunk;
}


/**
 * Create new memory chunk.
 *
 */
static void* cba_allocate_chunk(struct cba *cba, uint16_t idx, uint16_t len)
{
    uint16_t available;
    len = cba_chunk_len(len);

    if (idx < cba->tail_idx) {
        // We are wrapped already, check space till allocated memory
        available = cba->tail_idx - idx - CBA_END_MARKER_LEN;
        if ((len + CBA_HEADER_LEN) < available)
            return cba_reallocate_chunk(cba, idx, len);
    }
    else {
        // We are not wrapped
        // Check if there is space for the requested chunk without wrapping
        available = cba->size - idx - CBA_END_MARKER_LEN;
        if ((len + CBA_HEADER_LEN) < available)
            return cba_reallocate_chunk(cba, idx, len);

        // Check if there is space for the requested chunk at the beginning of the buffer (possible wrapping)
        if (cba->tail_idx > 0) {
            available = cba->tail_idx - CBA_END_MARKER_LEN;
            if ((len + CBA_HEADER_LEN) < available) {
                uint16_t free_len = cba->size - (cba->head_idx + CBA_HEADER_LEN);
                cba_reallocate_chunk(cba, cba->head_idx, free_len);
                return cba_reallocate_chunk(cba, cba->free_idx, len);
            }
        }
    }

    // out of memory
    return NULL;
}






/**
 * Initialize allocator structure.
 *
 */
void cba_init(struct cba *cba, void *buffer, uint16_t len)
{
    cba->free_idx = 0;
    cba->head_idx = 0;
    cba->tail_idx = 0;
    cba->buffer = buffer;
    cba->size = len;
}


/**
 * Clean allocator structure.
 *
 */
void cba_clean(struct cba *cba)
{
    cba->buffer = NULL;
    cba->size = 0;
}


/**
 * Reset allocator structure
 */
void cba_reset(struct cba *cba)
{
    cba->free_idx = 0;
    cba->head_idx = 0;
    cba->tail_idx = 0;
}


/**
 * Allocate new memory chunk.
 */
void* cba_malloc(struct cba *cba, uint16_t len)
{
    return cba_allocate_chunk(cba, cba->free_idx, len);
}


#ifdef CBA_REALLOC
/**
 * Reallocate existing memory chunk.
 *
 * Only last allocated chunk may be modified.
 *
 */
void* cba_realloc(struct cba *cba, void *chunk, uint16_t len)
{
    if (!chunk)
        return NULL;

    uint16_t idx = cba_chunk_idx(cba, chunk);
    struct cba_header *header = (struct cba_header*) &cba->buffer[idx];
    CBA_VALIDATE_HEAD_MARKER(header);

    if (header->chunk_len > len + CBA_END_MARKER_LEN)
        return chunk;   // Allocated chunk is big enough

    if (idx != cba->head_idx)
        return NULL;    // Only last allocated chunk may be reallocated

    uint16_t prev_len = header->chunk_len;
    void *new_chunk = cba_allocate_chunk(cba, cba->head_idx, len);
    if (new_chunk && (new_chunk != header->chunk)) {
        // New chunk allocated, relocation needed
        memmove(new_chunk, header->chunk, prev_len);
        cba_free(cba, header->chunk);
    }

    return new_chunk;
}
#endif // CBA_REALLOC


/**
 * Free memory chunk.
 *
 */
void* cba_free(struct cba *cba, void *chunk)
{
    if (!chunk)
        return NULL;

    uint16_t idx = cba_chunk_idx(cba, chunk);
    struct cba_header *header = (struct cba_header*) &cba->buffer[idx];
    CBA_VALIDATE_HEAD_MARKER(header);

    header->bound = 0;  // Unbound chunk

    if (idx == cba->tail_idx) {
        // Free all tailed unbound chunks
        while (idx != cba->free_idx) {
            header = (struct cba_header*) &cba->buffer[idx];
            CBA_VALIDATE_HEAD_MARKER(header);
            if (header->bound == 1)
                break;
            CBA_VALIDATE_END_MARKER(header);
            idx = cba_next_chunk_idx(cba, idx);
            cba->tail_idx = idx;
        }

        if (cba->tail_idx == cba->free_idx) {
            cba->tail_idx = 0;
            cba->head_idx = 0;
            cba->free_idx = 0;
        }
    }

    return NULL;
}




#ifdef CBA_PRINT
/**
 * Print information about given memory block.
 *
 */
void cba_print_chunk(struct cba_header *header)
{
    printf("       | %p    | %p\n", (void*)header, header->chunk);
    printf("          %3d:%d", header->chunk_len, header->bound);

    char *ptr = (char*)header;
    printf(" | ");
    for (int i=0; i<2; i++)
        printf("%02X", (unsigned char)(ptr[i]));
    printf(" | ");
    for (int i=2; i<4; i++)
        printf("%02X", (unsigned char)(ptr[i]));
    printf(" |");
    for (int i=4; i<header->chunk_len+4; i++)
        printf(" %02X", (unsigned char)(ptr[i]));

    printf(" |\n\n");
}

/**
 * Print information about given allocator.
 *
 */
void cba_print(struct cba *cba)
{
    printf("--- cba ---\n");
    printf("      %3d  %p\n", cba->size, cba->buffer);
    printf(" free %3d  %p\n", cba->free_idx, &cba->buffer[cba->free_idx]);
    printf(" head %3d  %p\n", cba->head_idx, &cba->buffer[cba->head_idx]);
    printf(" tail %3d  %p\n", cba->tail_idx, &cba->buffer[cba->tail_idx]);
    printf("---\n");

    uint16_t idx = cba->tail_idx;
    while (idx != cba->free_idx) {
        struct cba_header *header = (struct cba_header*)&cba->buffer[idx];
        printf("chunk %3d", idx);
        cba_print_chunk(header);

        idx = cba_next_chunk_idx(cba, idx);
    }
}
#endif //CBA_PRINT
