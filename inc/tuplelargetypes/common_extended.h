#ifndef COMMON_EXTENDED_H
#define COMMON_EXTENDED_H

#include<tuplestore/tuple.h>
#include<tuplestore/data_type_info.h>
#include<tupleindexer/common/page_access_specification.h>

// anything that is not extended is inline, including float, integer, etc
int is_inline_type_info(const data_type_info* dti_p);
int is_extended_type_info(const data_type_info* dti_p);

// returns 1-th element from the large type info
uint64_t get_extension_head_page_id_for_extended_type(const void* tupl, const tuple_def* tpl_d, positional_accessor pa, const page_access_specs* pas_p);

#endif