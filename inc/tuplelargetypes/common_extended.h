#ifndef COMMON_EXTENDED_H
#define COMMON_EXTENDED_H

#include<tuplestore/tuple.h>
#include<tuplestore/data_type_info.h>
#include<tupleindexer/common/page_access_specification.h>

#define EXTENDED_TYPE_SUFFIX        "_extended"
#define INLINE_TYPE_SUFFIX          "_inline"

#define EXTENDED_PREFIX_POS_ACC          STATIC_POSITION(0)

#define EXTENDED_HEAD_PAGE_ID_POS_ACC    STATIC_POSITION(1)

// anything that is not extended is inline, including float, integer, etc
int is_inline_type_info(const data_type_info* dti_p);
int is_extended_type_info(const data_type_info* dti_p);

// returns 1-th element from the large type info
uint64_t get_extension_head_page_id_for_extended_type(const datum* uval, const data_type_info* dti, const page_access_specs* pas_p);

#endif