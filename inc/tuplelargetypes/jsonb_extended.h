#ifndef JSONB_EXTENDED_H
#define JSONB_EXTENDED_H

#include<tuplestore/data_type_info.h>
#include<tupleindexer/common/page_access_specification.h>

// below one function only check the type_name of the dti passed
int is_jsonb_extended_type_info(const data_type_info* dti_p);

// returns a new type info pointing to a tuple, of 2 elements the blob inline, and a page_id type from pas_p
data_type_info* get_jsonb_extended_type_info(uint32_t max_size, uint32_t inline_size, const page_access_specs* pas_p);

#include<tuplelargetypes/common_extended.h>

#include<jsonb_node.h>
#include<jsonb_parser.h>
#include<jsonb_serializer.h>

#include<tuplelargetypes/binary_read_iterator.h>
#include<tuplelargetypes/binary_write_iterator.h>

#endif