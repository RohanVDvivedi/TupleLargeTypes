#ifndef COMMON_EXTENDED_H
#define COMMON_EXTENDED_H

#include<tuplestore/tuple.h>
#include<tuplestore/data_type_info.h>

#include<tupleindexer/common/page_access_specification.h>
#include<tupleindexer/common/tuple_pointer.h>

#define EXTENDED_TYPE_SUFFIX                 "extended"
#define INLINE_TYPE_SUFFIX                   "inline"

#define EXTENDED_PREFIX_POS_VAL               (0)
#define EXTENDED_PREFIX_POS_ACC               STATIC_POSITION(EXTENDED_PREFIX_POS_VAL)

#define EXTENSION_HEAD_POS_ACC                STATIC_POSITION(1)

// anything that is not extended is inline, including float, integer, etc
int is_inline_type_info(const data_type_info* dti_p);

int is_extended_type_info(const data_type_info* dti_p);

int has_extended_type_info(const data_type_info* dti_p);

int has_extended_type_info2(const tuple_def* tpl_d, positional_accessor pos);

int has_extended_type_info3(const tuple_def* tpl_d, uint32_t key_element_count, const positional_accessor* key_element_ids);

// returns true, if any of the constituent types of this type info is an extended type info
int has_extended_type_info(const data_type_info* dti_p);

// returns 1-th element from the large type info
tuple_pointer get_extension_head_for_extended_type(const datum* uval, const data_type_info* dti, const page_access_specs* pas_p);

void set_extension_head_for_extended_type(void* tupl, const tuple_def* tpl_d, positional_accessor pos, const page_access_specs* pas_p, tuple_pointer cptr);

#endif