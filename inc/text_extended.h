#ifndef TEXT_EXTENDED_H
#define TEXT_EXTENDED_H

#include<data_type_info.h>
#include<page_access_specification.h>

// below two functions only check the type_name of the dti passed
int is_text_inline_type_info(const data_type_info* dti_p);
int is_text_extended_type_info(const data_type_info* dti_p);

// returns a new type info pointing to STRING type that is atmost max_size bytes big
data_type_info* get_text_inline_type_info(uint32_t max_size);

// returns a new type info pointing to a tuple, of 2 elements the text_inline_p, and a page_id type from pas_p
data_type_info* get_text_extended_type_info(const data_type_info* text_inline_p, const page_access_specs* pas_p);

#include<common_extended.h>

#include<text_blob_read_iterator.h>
#include<text_blob_write_iterator.h>

#endif