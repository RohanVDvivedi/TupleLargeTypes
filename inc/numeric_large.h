#ifndef NUMERIC_LARGE_H
#define NUMERIC_LARGE_H

#include<data_type_info.h>
#include<page_access_specification.h>

// below two functions only check the type_name of the dti passed
int is_numeric_short_type_info(const data_type_info* numeric_short_p);
int is_numeric_large_type_info(const data_type_info* numeric_large_p);

// returns a new type info pointing to short type that is atmost max_size bytes big
// max_size = 2 * ceil(log(page_size) base 256) + 1 (bytes for bit field) + 2 (bytes for exponent) + 5 * ceil(digits / 12)
// for 8 KB page and storing 36 digits, max_size = 2 * 2 + 1 + 2 + 5 * 3 = 22 bytes of max size
data_type_info* get_numeric_short_type_info(uint32_t max_size);

// returns a new type info pointing to a tuple, of 2 elements the numeric_short_p, and a page_id type from pas_p
data_type_info* get_numeric_large_type_info(const data_type_info* numeric_short_p, const page_access_specs* pas_p);

#endif