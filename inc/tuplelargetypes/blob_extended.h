#ifndef BLOB_EXTENDED_H
#define BLOB_EXTENDED_H

#include<tuplestore/data_type_info.h>
#include<tupleindexer/common/page_access_specification.h>

// below two functions only check the type_name of the dti passed
int is_blob_inline_type_info(const data_type_info* dti_p);
int is_blob_extended_type_info(const data_type_info* dti_p);

// returns a new type info pointing to BLOB type that is atmost max_size bytes big
data_type_info* get_blob_inline_type_info(uint32_t max_size);

// returns a new type info pointing to a tuple, of 2 elements the blob_inline_p, and a page_id type from pas_p
data_type_info* get_blob_extended_type_info(uint32_t max_size, const data_type_info* blob_inline_p, const page_access_specs* pas_p);

#include<tuplelargetypes/common_extended.h>

#include<tuplelargetypes/binary_read_iterator.h>
#include<tuplelargetypes/binary_write_iterator.h>

#include<tuplelargetypes/binary_hasher.h>
#include<tuplelargetypes/binary_comparator.h>

#endif