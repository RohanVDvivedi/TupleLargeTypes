#ifndef BINARY_READ_ITERATOR_H
#define BINARY_READ_ITERATOR_H

#include<tuple.h>
#include<worm.h>
#include<common_extended.h>

/*
	To be used solely with data_type_info-s that are structurally similar to
	inline types like : text_inline, blob_inline, 
	extended types like : text_extended, blob_extended,

	It gives you a TupleIndexer worm like interface, that means you can easily use consuming inline and overflow page contents in a stream like fashion agnostic of whether it is an inline type or an extended type
*/

typedef struct binary_read_iterator binary_read_iterator;
struct binary_read_iterator
{
	int is_inline:1;

	// local copy of the prefix and the bytes that have been read from it already
	user_value prefix;
	uint32_t bytes_read_from_prefix;

	// head_page_id to start the worm, unused if is_inline is set
	uint64_t extension_head_page_id;

	// above three attributes are cached in the constructor

	// unused if is_inline is set
	worm_read_iterator* wri_p;

	// below attributes only to be used to initialize the wri, only upon requirement
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
};

binary_read_iterator* get_new_binary_read_iterator(const void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p);

void delete_binary_read_iterator(binary_read_iterator* tbri_p, const void* transaction_id, int* abort_error);

uint32_t read_from_text_blob(binary_read_iterator* tbri_p, char* data, uint32_t data_size, const void* transaction_id, int* abort_error);

binary_read_iterator* clone_binary_read_iterator(binary_read_iterator* tbri_p, const void* transaction_id, int* abort_error);

#endif