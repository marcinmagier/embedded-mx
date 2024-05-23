
#ifndef __MX_STRING_H_
#define __MX_STRING_H_


#include <string.h>
#include <stdbool.h>




static inline bool is_upper(char c)
{
    return (c >= 'A' && c <= 'Z');
}


static inline bool is_alpha(char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}


static inline bool is_space(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}


static inline bool is_digit(char c)
{
    return (c >= '0' && c <= '9');
}



const char* str_strip_left(const char *str);

bool str_equals(const char *str, const char *pattern);
bool str_starts_with(const char *str, const char *pattern, const char **followup);

bool str_to_long(const char *str, long *val, int base);




#endif /* __MX_STRING_H_ */
