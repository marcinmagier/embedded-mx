
#include "mx/string.h"





const char* str_strip_left(const char *str)
{
    while (*str && is_space(*str))
        str++;

    return str;
}


bool str_equals(const char *str, const char *pattern)
{
    return strcmp(str, pattern) == 0 ? true : false;
}


bool str_starts_with(const char *str, const char *pattern, const char **followup)
{

    size_t pattern_len = strlen(pattern);

    if (strncmp(str, pattern, pattern_len))
        return false;

    if (followup)
        *followup = str + pattern_len;

    return true;
}


bool str_to_long(const char *str, long *val, int base)
{
    long retval = 0;
    int sign = 1;
    int c;

    /*
     * Skip white space and pick up leading +/- sign if any.
     */
    str = str_strip_left(str);
    if (*str == '-') {
        sign = -1;
        str++;
    }
    else if (*str == '+') {
        str++;
    }

    /*
     * If base is 0, allow 0x for hex and 0 for octal, else
     * assume decimal; if base is already 16, allow 0x.
     */
    if ((base == 0 || base == 16) &&
        str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        base = 16;
    }
    if (base == 0)
        base = (*str == '0') ? 8 : 10;

    /*
     * Convert string to number
     */
    while (c = *str++, c) {
        if (is_digit(c))
            c -= '0';
        else if (is_alpha(c))
            c -= is_upper(c) ? 'A' - 10 : 'a' - 10;
        else
            return false;
        if (c >= base)
            return false;

        retval *= base;
        retval += c;
    }

    *val = sign * retval;
    return true;
}

