
#ifndef __MX_ATTR_H_
#define __MX_ATTR_H_


#undef __attr_malloc
#undef __attr_align
#undef __attr_alloc_size
#undef __attr_alloc_size_prod
#undef __attr_alloc_align
#undef __attr_pure
#undef __attr_const
#undef __attr_unused_result
#undef __attr_always_inline
#undef __attr_unused
#undef __attr_nonnull_all
#undef __attr_nonnull_arg
#undef __attr_nonnull_ret



#ifndef DEFINED_REAL_ATTR
  #define DEFINED_REAL_ATTR
  #ifdef __GNUC__
    // common
    #define __real_attr_malloc                  __attribute__((malloc))
    #define __real_attr_align(n)                __attribute__((__aligned__(n)))
    #define __real_attr_alloc_align(x)          __attribute__((alloc_align(x)))
    #define __real_attr_pure                    __attribute__((pure))
    #define __real_attr_const                   __attribute__((const))
    #define __real_attr_unused_result           __attribute__((warn_unused_result))
    #define __real_attr_always_inline           __attribute__((always_inline))
    #define __real_attr_unused                  __attribute__((unused))
    #define __real_attr_nonnull_all             __attribute__((nonnull))
    #define __real_attr_nonnull_arg(...)        __attribute__((nonnull(__VA_ARGS__)))

    #ifdef __clang__
      // clang only
    #elif defined(__INTEL_COMPILER)
      // intel only
    #else
      #define GCC_VERSION  (__GNUC__ * 10000 +  __GNUC_MINOR__ * 100 +  __GNUC_PATCHLEVEL__)
      // gcc only
      #define __real_attr_alloc_size(x)         __attribute__((alloc_size(x)))
      #define __real_attr_alloc_size_prod(x,y)  __attribute__((alloc_size(x,y)))
      #if GCC_VERSION >= 40900
        #define __real_attr_nonnull_ret         __attribute__((returns_nonnull))
      #endif
    #endif
  #endif

  // define missing function attributes

  #ifndef __real_attr_malloc
    #define __real_attr_malloc
  #endif

  #ifndef __real_attr_align
    #define __real_attr_align(n)
  #endif

  #ifndef __real_attr_alloc_size
    #define __real_attr_alloc_size(x)
  #endif

  #ifndef __real_attr_alloc_size_prod
    #define __real_attr_alloc_size_prod(x,y)
  #endif

  #ifndef __real_attr_alloc_align
    #define __real_attr_alloc_align(x)
  #endif

  #ifndef __real_attr_pure
    #define __real_attr_pure
  #endif

  #ifndef __real_attr_const
    #define __real_attr_const
  #endif

  #ifndef __real_attr_unused_result
    #define __real_attr_unused_result
  #endif

  #ifndef __real_attr_always_inline
    #define __real_attr_always_inline
  #endif

  #ifndef __real_attr_unused
    #define __real_attr_unused
  #endif

  #ifndef __real_attr_nonnull_all
    #define __real_attr_nonnull_all
  #endif

  #ifndef __real_attr_nonnull_arg
    #define __real_attr_nonnull_arg(...)
  #endif

  #ifndef __real_attr_nonnull_ret
    #define __real_attr_nonnull_ret
  #endif
#endif




#define __attr_malloc                 __real_attr_malloc
#define __attr_align(n)               __real_attr_align(n)
#define __attr_alloc_size(x)          __real_attr_alloc_size(x)
#define __attr_alloc_size_prod(x,y)   __real_attr_alloc_size_prod(x,y)
#define __attr_alloc_align(x)         __real_attr_alloc_align(x)
#define __attr_pure                   __real_attr_pure
#define __attr_const                  __real_attr_const
#define __attr_unused_result          __real_attr_unused_result
#define __attr_always_inline          __real_attr_always_inline
#define __attr_unused                 __real_attr_unused
#define __attr_nonnull_all            __real_attr_nonnull_all
#define __attr_nonnull_arg(...)       __real_attr_nonnull_arg(__VA_ARGS__)
#define __attr_nonnull_ret            __real_attr_nonnull_ret


#endif /* __MX_ATTR_H_ */
