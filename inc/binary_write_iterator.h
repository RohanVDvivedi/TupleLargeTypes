#ifndef BINARY_WRITE_ITERATOR_H
#define BINARY_WRITE_ITERATOR_H

#include<tuple.h>
#include<worm.h>
#include<common_extended.h>

/*
	To be used solely with data_type_info-s that are structurally similar to
	inline types like : text_inline, blob_inline, 
	extended types like : text_extended, blob_extended,

	It gives you a TupleIndexer worm like interface, that means you can easily use consuming inline and overflow page contents in a stream like fashion agnostic of whether it is an inline type or an extended type

	Note: there should not exist another binary_write_iterator on the same attribute of the same tuple at the same time
*/

typedef struct binary_write_iterator binary_write_iterator;
struct binary_write_iterator
{
	int is_inline:1;

	// shallow copy attributes
	void* tupl;
	const tuple_def* tpl_d;
	positional_accessor inline_accessor;

	uint32_t bytes_to_be_written_to_prefix;
	uint32_t bytes_written_to_prefix;
	// no more appending to prefix if the above 2 attributes become equal

	// unused if is_inline is set
	worm_append_iterator* wai_p;

	// below attributes only to be used to initialize the wai, only upon requirement
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
	const page_modification_methods* pmm_p;
};

/*
	prefix is appended to a non-persistent tuple, making it as much larger as possible (also controller using bytes_to_be_written_to_prefix parameter)
	suffix is transaction protected, and could be persistent
*/

// the text of blob in context must be either NULL or atleast EMPTY
binary_write_iterator* get_new_binary_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor, uint32_t bytes_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p);

void delete_binary_write_iterator(binary_write_iterator* bwi_p, const void* transaction_id, int* abort_error);

uint32_t append_to_binary_write_iterator(binary_write_iterator* bwi_p, const char* data, uint32_t data_size, const void* transaction_id, int* abort_error);

#endif