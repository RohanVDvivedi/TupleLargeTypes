#ifndef BINARY_WRITE_ITERATOR_H
#define BINARY_WRITE_ITERATOR_H

#include<tuplestore/tuple.h>
#include<tupleindexer/worm/worm.h>
#include<tuplelargetypes/common_extended.h>

/*
	To be used solely with data_type_info-s that are structurally similar to
	inline types like : text_inline, binary_inline, 
	extended types like : text_extended, binary_extended,

	It gives you a TupleIndexer worm like interface, that means you can easily use consuming inline and overflow page contents in a stream like fashion agnostic of whether it is an inline type or an extended type

	Note: there should not exist another binary_write_iterator on the same tuple at the same instant
*/

typedef struct binary_write_iterator binary_write_iterator;
struct binary_write_iterator
{
	int is_extended:1;

	// shallow copy of input parameters
	void* tupl;
	const tuple_def* tpl_d;
	positional_accessor pos;

	uint32_t bytes_written_to_prefix;
	uint32_t bytes_to_be_written_to_prefix;
	// no more appending to prefix if the above 2 attributes become equal

	// used only if is_extended is set
	worm_append_iterator* wai_p;

	// below attributes only to be used to initialize the wai, only upon requirement
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
	const page_modification_methods* pmm_p;
};

// the text or binary in context must be EMPTY_DATUM or partially written
binary_write_iterator* get_new_binary_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor pos, uint32_t bytes_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p);

void delete_binary_write_iterator(binary_write_iterator* bwi_p, const void* transaction_id, int* abort_error);

uint32_t append_to_binary_write_iterator(binary_write_iterator* bwi_p, const char* data, uint32_t data_size, const void* transaction_id, int* abort_error);

// below header can allow you to wrap the iterator under a stream interface
#include<tuplelargetypes/binary_writer_stream.h>

#endif