#ifndef COMMON_EXTENDED_H
#define COMMON_EXTENDED_H

#include<tuplestore/tuple.h>
#include<tuplestore/data_type_info.h>
#include<tupleindexer/common/page_access_specification.h>

#define EXTENDED_TYPE_SUFFIX                 "_extended"
#define INLINE_TYPE_SUFFIX                   "_inline"

#define EXTENDED_PREFIX_POS_VAL               (0)
#define EXTENDED_PREFIX_POS_ACC               STATIC_POSITION(EXTENDED_PREFIX_POS_VAL)

#define EXTENSION_HEAD_POS_ACC                STATIC_POSITION(1)

#define EXTENSION_HEAD_PAGE_ID_POS_ACC        STATIC_POSITION(1, 0)
#define EXTENSION_HEAD_TUPLE_INDEX_POS_ACC    STATIC_POSITION(1, 1)

typedef struct chunk_ptr chunk_ptr;
struct chunk_ptr
{
	uint64_t page_id;
	uint32_t tuple_index;
	uint32_t byte_index;
};

// anything that is not extended is inline, including float, integer, etc
int is_inline_type_info(const data_type_info* dti_p);
int is_extended_type_info(const data_type_info* dti_p);

// returns true, if any of the constituent types of this type info is an extended type info
int has_extended_type_info(const data_type_info* dti_p);

// returns 1-th element from the large type info
chunk_ptr get_extension_head_for_extended_type(const datum* uval, const data_type_info* dti, const page_access_specs* pas_p);

#include<tupleindexer/blob_store/blob_store.h>

void initialize_extension_head(void* tupl, const tuple_def* tpl_d, positional_accessor pos, const page_access_specs* pas_p);

#endif