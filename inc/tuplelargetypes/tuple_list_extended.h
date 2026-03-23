#ifndef TUPLE_LIST_EXTENDED_H
#define TUPLE_LIST_EXTENDED_H

#include<tuplestore/data_type_info.h>
#include<tupleindexer/common/page_access_specification.h>

/*
	The tuple_list_extended type info is nothing but a list of tuples inside a blob_extended like type
	it stores tuples one after another in the prefix then in the worm, adjacently
	all these tuples can be of the same type or different, the api does not enforce this
	the user is responsible to provide the tuple_def that deciphers the ith tuple in the row
	it is your responsibility to provide the respective tuple_def for using it with the tuple_list_extended
*/

#define TUPLE_LIST_TYPE_PREFIX "tuple_list"

// below one function only check the type_name of the dti passed
int is_tuple_list_extended_type_info(const data_type_info* dti_p);

// returns a new type info pointing to a tuple, of 2 elements the binary inline, and a page_id type from pas_p
data_type_info* get_tuple_list_extended_type_info(uint32_t max_size, uint32_t inline_size, const page_access_specs* pas_p);

#include<tuplelargetypes/common_extended.h>

#include<tuplelargetypes/binary_read_iterator.h>
#include<tuplelargetypes/binary_write_iterator.h>

#include<tuplelargetypes/tuple_list_helper.h>

#endif