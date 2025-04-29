#ifndef BINARY_READ_ITERATOR_H
#define BINARY_READ_ITERATOR_H

#include<tuplestore/tuple.h>
#include<tupleindexer/worm/worm.h>
#include<tuplelargetypes/common_extended.h>

/*
	To be used solely with data_type_info-s that are structurally similar to
	inline types like : text_inline, blob_inline, 
	extended types like : text_extended, blob_extended,

	It gives you a TupleIndexer worm like interface, that means you can easily use consuming inline and overflow page contents in a stream like fashion agnostic of whether it is an inline type or an extended type

	Note: there should not exist another binary_write_iterator on the same attribute of the same tuple at the same time
*/

typedef struct binary_read_iterator binary_read_iterator;
struct binary_read_iterator
{
	int is_inline:1;

	// shallow copy attributes
	const void* tupl;
	const tuple_def* tpl_d;
	positional_accessor inline_accessor;

	uint32_t bytes_read_from_prefix;
	// no reading bytes from prefix, if this value becomes equal to the prefix size

	// unused if is_inline is set
	worm_read_iterator* wri_p;

	// below attributes only to be used to initialize the wri, only upon requirement
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
};

binary_read_iterator* get_new_binary_read_iterator(const void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p);

void delete_binary_read_iterator(binary_read_iterator* bri_p, const void* transaction_id, int* abort_error);

uint32_t read_from_binary_read_iterator(binary_read_iterator* bri_p, char* data, uint32_t data_size, const void* transaction_id, int* abort_error);

// does not move the iterator forward but allows you to peek some non-zero bytes into the future
// if (*data_size) returns 0, then you are at the end of the data
const char* peek_in_binary_read_iterator(binary_read_iterator* bri_p, uint32_t* data_size, const void* transaction_id, int* abort_error);

binary_read_iterator* clone_binary_read_iterator(const binary_read_iterator* bri_p, const void* transaction_id, int* abort_error);

#include<tuplelargetypes/binary_read_iterator_stream.h>

#endif