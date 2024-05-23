
#ifndef __MX_MISC_H_
#define __MX_MISC_H_

#include <stdint.h>



#undef NULL
#ifdef __cplusplus
  #define NULL 0
#else
  #define NULL ((void *)0)
#endif



typedef int64_t   s64_t;
typedef int32_t   s32_t;
typedef int16_t   s16_t;
typedef int8_t    s8_t;
typedef uint64_t  u64_t;
typedef uint32_t  u32_t;
typedef uint16_t  u16_t;
typedef uint8_t   u8_t;

typedef uint8_t   bits_t;
//typedef uint32_t  size_t;
//typedef uint32_t  time_t;
typedef int8_t*   string_t;

typedef uint8_t   bool_t;
enum {FALSE = 0, TRUE = !FALSE};

typedef uint8_t   status_t;
enum {RESET = 0, SET = !RESET};
enum {DISABLED = 0, ENABLED = !DISABLED};

typedef uint8_t   fmode_t;
enum {NONBLOCK = FALSE, BLOCK = TRUE};

typedef uint8_t   result_t;
enum {
  SUCCESS = 0,
  ERROR,
  ERR_BUFFER_EMPTY,
  ERR_BUFFER_FULL,
  ERR_DEV_NOT_CONFIGURED,
  ERR_DEV_BUSY,
  ERR_ITEM_NOT_FOUND
};

typedef void (*callback_t)(u32_t);



#define UNUSED(x)       { (void)(x); }
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))

#ifndef MAX
#define MAX(a, b)       (((a) < (b)) ? (b) : (a))
#endif

#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)          (((a) < 0) ? -(a) : (a))
#endif

#define _STR(s)         #s
#define STR(s)          _STR(s)

#define _JOIN(x, y)     x ## y
#define JOIN(x, y)      _JOIN(x, y)

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))


#define SET_BIT(reg, bit)     ((reg) |= (bit))
#define CLEAR_BIT(reg, bit)   ((reg) &= ~(bit))
#define READ_BIT(reg, bit)    ((reg) & (bit))
#define CLEAR_REG(reg)        ((reg) = (0x0))
#define WRITE_REG(reg, val)   ((reg) = (val))
#define READ_REG(reg)         ((reg))
#define MODIFY_REG(reg, clearmask, setmask)  WRITE_REG((reg), (((READ_REG(reg)) & (~(clearmask))) | (setmask)))


#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - offsetof(type,member) );})




#if defined ( __CC_ARM   )
  //      __asm       is defined by default
  #define __code    const
  //      __inline    is defined by default
  #define __weak    __attribute__((weak))
  #define __ramfunc __attribute__((section ("ram_code")))
#elif defined ( __ICCARM__ )
  //      __asm    is defined by default
  #define __code    const
  #define __inline  inline
  //      __weak      is defined by default
  //      __ramfunc   is defined by default
#elif defined   (  __GNUC__  )
  //      __asm       is defined by default
  #define __code    const
  #define __inline  inline
  #define __weak    __attribute__((weak))
  #define __ramfunc __attribute__((section (".ram")))
#else
  #error Unknown compiler
#endif



#endif /* __MX_MISC_H_ */
