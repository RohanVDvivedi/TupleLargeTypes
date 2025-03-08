#ifndef COMMON_LARGE_H
#define COMMON_LARGE_H

#include<tuple.h>
#include<data_type_info.h>

int is_short_type_info(const data_type_info* dti_p);
int is_large_type_info(const data_type_info* dti_p);

// this implies a large type is uninitialized and so it's worm page_id is just uninitialized and must not be touched
int is_prefix_valid_and_not_NULL(const data_type_info* dti_p, positional_accessor pa);

// returns 1-th element from the large type info
uint64_t get_extension_head_page_id(const data_type_info* dti_p, positional_accessor pa);

#endif