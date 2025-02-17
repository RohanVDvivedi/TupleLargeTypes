#ifndef TEXT_LARGE_H
#define TEXT_LARGE_H

#include<data_type_info.h>
#include<page_access_specification.h>

// returns a new type info pointing to to a tuple, of 2 elements the text_short_p, and a page_id type from pas_p
data_type_info* get_text_large_type_info(const data_type_info* text_short_p, const page_access_specs* pas_p);

#endif