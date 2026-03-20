#ifndef COMMON_EXTENDED_H
#define COMMON_EXTENDED_H

#include<tuplestore/tuple.h>
#include<tuplestore/data_type_info.h>
#include<tupleindexer/common/page_access_specification.h>

#define EXTENDED_TYPE_SUFFIX        "_extended"
#define INLINE_TYPE_SUFFIX          "_inline"

#define EXTENDED_PREFIX_POS_VAL          (0)
#define EXTENDED_PREFIX_POS_ACC          STATIC_POSITION(EXTENDED_PREFIX_POS_VAL)

#define EXTENDED_HEAD_PAGE_ID_POS_ACC    STATIC_POSITION(1)

// anything that is not extended is inline, including float, integer, etc
int is_inline_type_info(const data_type_info* dti_p);
int is_extended_type_info(const data_type_info* dti_p);

// returns 1-th element from the large type info
uint64_t get_extension_head_page_id_for_extended_type(const datum* uval, const data_type_info* dti, const page_access_specs* pas_p);

#include<tupleindexer/worm/worm.h>

// creates a new worm, and inserts it's head_page_id in the tupe at pos position and will also return it
// does not create a new one if one already exists in the tuple for the pos position
uint64_t get_or_create_extension_worm(void* tupl, const tuple_def* tpl_d, positional_accessor pos, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p, const void* transaction_id, int* abort_error);

void delete_all_extension_worms(const datum* uval, const data_type_info* dti, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p, const void* transaction_id, int* abort_error);

#endif