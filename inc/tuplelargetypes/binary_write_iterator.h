#ifndef BINARY_WRITE_ITERATOR_H
#define BINARY_WRITE_ITERATOR_H

#include<tuplestore/tuple.h>
#include<tupleindexer/blob_store/blob_store.h>
#include<tuplelargetypes/common_extended.h>

/*
	to be used solely with data_type_info-s that are structurally similar to
	inline types like : text_inline, binary_inline, 
	extended types like : text_extended, binary_extended,

	it gives you a TupleIndexer blob_store like interface, that means you can easily use append it's inline and overflow page contents in a stream like fashion agnostic of whether it is an inline type or an extended type

	Note: there should not exist another binary_write_iterator on the same tuple at the same instant
*/

typedef struct binary_write_iterator binary_write_iterator;
struct binary_write_iterator
{
	unsigned int is_extended:1;

	// shallow copy of input parameters
	void* tupl;
	const tuple_def* tpl_d;
	positional_accessor pos;
	uint64_t blob_store_root_page_id;

	// set to right value after every insert
	tuple_pointer extension_head;
	tuple_pointer extension_tail;

	uint32_t bytes_written_to_prefix;
	uint32_t bytes_to_be_written_to_prefix;
	// no more appending to prefix if the above 2 attributes become equal

	// used only if is_extended is set
	blob_store_write_iterator* bswi_p;

	// below attributes only to be used to initialize the bswi, only upon requirement
	const blob_store_tuple_defs* bstd_p;
	const page_access_methods* pam_p;
	const page_modification_methods* pmm_p;
};

// the text or binary in context must be set to EMPTY_DATUM or partially written
binary_write_iterator* get_new_binary_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor pos, uint64_t blob_store_root_page_id, tuple_pointer extension_tail, uint32_t bytes_to_be_written_to_prefix, const blob_store_tuple_defs* bstd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p);

void delete_binary_write_iterator(binary_write_iterator* bwi_p, const void* transaction_id, int* abort_error);

uint32_t append_to_binary_write_iterator(binary_write_iterator* bwi_p, const char* data, uint32_t data_size, const heap_table_notifier* notify_wrong_entry, const void* transaction_id, int* abort_error);

#endif