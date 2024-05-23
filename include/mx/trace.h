
#ifndef __MX_TRACE_H_
 #define __MX_TRACE_H_


// ***** Includes *********************************************************************************

// ***** Defines and Enums ************************************************************************

  #define TRACE_LEVEL_DEBUG      5
  #define TRACE_LEVEL_INFO       4
  #define TRACE_LEVEL_WARNING    3
  #define TRACE_LEVEL_ERROR      2
  #define TRACE_LEVEL_FATAL      1
  #define TRACE_LEVEL_NO_TRACE   0

#ifdef NDEBUG

  #undef TRACE_LEVEL
  #define TRACE_LEVEL TRACE_LEVEL_NO_TRACE

#elif !defined(TRACE_LEVEL)    // By default, all traces are output except the debug one.

  #define TRACE_LEVEL TRACE_LEVEL_WARNING

#endif

#if defined(NOTRACE)
  #error "Error: NOTRACE has to be not defined !"
#endif

#undef NOTRACE
#if (TRACE_LEVEL == TRACE_LEVEL_NO_TRACE)
  #define NOTRACE
#endif



// ***** Macros ***********************************************************************************

#if defined(NOTRACE)

  #define trace_init(...)       { }

  #define TRACE(...)            { }
  #define TRACE_DEBUG(...)      { }
  #define TRACE_INFO(...)       { }
  #define TRACE_WARNING(...)    { }
  #define TRACE_ERROR(...)      { }
  #define TRACE_PANIC(...)      { while(1); }

#else

  #define trace_init(...)       { trace_configure(); }

#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
  #define TRACE_DEBUG(...)      { trace_print("<D> " __VA_ARGS__); trace_print("\n"); }
#else
  #define TRACE_DEBUG(...)      { }
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_INFO)
  #define TRACE(...)            { trace_print(__VA_ARGS__); }
  #define TRACE_INFO(...)       { trace_print("<I> " __VA_ARGS__); trace_print("\n"); }
  #define TRACE_DATA(...)       { trace_data(__VA_ARGS__); }
#else
  #define TRACE(...)            { }
  #define TRACE_INFO(...)       { }
  #define TRACE_DATA(...)       { }
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_WARNING)
  #define TRACE_WARNING(...)    { trace_print("<W> " __VA_ARGS__); trace_print("\n"); }
#else
  #define TRACE_WARNING(...)    { }
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_ERROR)
  #define TRACE_ERROR(...)      { trace_print("<E> " __VA_ARGS__); trace_print("\n"); }
#else
  #define TRACE_ERROR(...)      { }
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_FATAL)
  #define TRACE_PANIC(...)      { trace_print("<P> " __VA_ARGS__); trace_print("\n"); while(1); }
#else
  #define TRACE_PANIC(...)      { while(1); }
#endif

#endif



// ***** Typedefs *********************************************************************************

// ***** Declaration of Variables *****************************************************************

// ***** Prototypes of Functions ******************************************************************


#ifndef NDEBUG
    void trace_configure(void);
    void trace_data(const char *prefix, unsigned char *data, unsigned long data_len);

  #ifdef TRACE_PRINTF
    // Fallback to standard printf function
    #include <stdio.h>
    #define trace_print(...) { printf(__VA_ARGS__); }
  #else
    // Custom trace_print
    int trace_print(const char *format, ...);
  #endif
#endif




#endif /* __MX_TRACE_H_ */

