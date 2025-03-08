#ifndef COMMON_LARGE_H
#define COMMON_LARGE_H

#include<tuple.h>
#include<data_type_info.h>
#include<page_access_specification.h>

int is_short_type_info(const data_type_info* dti_p);
int is_large_type_info(const data_type_info* dti_p);

// returns 1-th element from the large type info
uint64_t get_extension_head_page_id_for_large_type(const void* tupl, const tuple_def* tpl_d, positional_accessor pa, const page_access_specs* pas_p);

#endif