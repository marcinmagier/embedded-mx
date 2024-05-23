
#ifndef __MX_SANITY_CHECK_H_
 #define __MX_SANITY_CHECK_H_


// ***** Includes *********************************************************************************

 #include "trace.h"



// ***** Defines and Enums ************************************************************************

#ifdef NDEBUG
  #undef NOASSERT
  #define NOASSERT
#endif



// ***** Macros ***********************************************************************************

#ifdef NOASSERT
  #define ASSERT(...)
  #define SANITY_CHECK(...)
#else

  #if (TRACE_LEVEL == 0)
    #define ASSERT(condition, ...)  { if (!(condition)) {while(1);} }
    #define SANITY_CHECK(condition) ASSERT(condition, ...)

  #else
    #define ASSERT(condition, ...)  { if (!(condition)) { \
                                        trace_print("<ASSERT>: "); \
                                        trace_print(__VA_ARGS__); \
                                        trace_print("\n"); \
                                        while (1); \
                                    } }
    #define SANITY_ERROR            "Sanity check failed at %s:%d"
    #define SANITY_CHECK(condition) ASSERT(condition, SANITY_ERROR, __FILE__, __LINE__)
  #endif
#endif



// ***** Typedefs *********************************************************************************

// ***** Declaration of Variables *****************************************************************

// ***** Prototypes of Functions ******************************************************************






#endif /* __MX_SANITY_CHECK_H_ */

