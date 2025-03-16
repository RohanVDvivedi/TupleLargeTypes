#ifndef TUPLE_LIST_EXTENDED_H
#define TUPLE_LIST_EXTENDED_H

#include<data_type_info.h>
#include<page_access_specification.h>

/*
	The tuple_list_extended type info is nothing but a list of tuples inside a blob_extended like type
	it stores tuples one after another in the prefix then in the worm, adjacently
	All these tuples can be of the same type or different
	The user is responsible to provide the tuple_def that deciphers the ith tuple in the row
	It is your responsibility to not allow any tuple in here that have any worm extended type in here, directly or nestedly
*/

// below one function only check the type_name of the dti passed
int is_tuple_list_extended_type_info(const data_type_info* dti_p);

// returns a new type info pointing to a tuple, of 2 elements the blob inline, and a page_id type from pas_p
data_type_info* get_tuple_list_extended_type_info(uint32_t inline_size, const page_access_specs* pas_p);

#include<common_extended.h>

#include<binary_read_iterator.h>
#include<binary_write_iterator.h>

#include<tuple_list_helper.h>

#endif