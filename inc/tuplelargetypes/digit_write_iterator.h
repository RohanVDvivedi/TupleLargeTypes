#ifndef DIGIT_WRITE_ITERATOR_H
#define DIGIT_WRITE_ITERATOR_H

#include<tuplestore/tuple.h>
#include<tupleindexer/blob_store/blob_store.h>
#include<tuplelargetypes/common_extended.h>

/*
	no be used solely with data_type_info-s that are structurally similar to
	inline types like : numeric_inline,
	extended types like : numeric_extended,

	note: there should not exist another digit_write_iterator on the same attribute of the same tuple at the same time
*/

typedef struct digit_write_iterator digit_write_iterator;
struct digit_write_iterator
{
	int is_extended:1;

	// shallow copy of input parameters
	void* tupl;
	const tuple_def* tpl_d;
	positional_accessor pos;
	uint64_t blob_store_root_page_id;

	// set to right value after every insert
	tuple_pointer extension_head;
	tuple_pointer extension_tail;

	uint32_t digits_written_to_prefix;
	uint32_t digits_to_be_written_to_prefix;
	// no more appending to prefix if the above 2 attributes become equal

	// used only if is_extended is set
	blob_store_write_iterator* bswi_p;

	// below attributes only to be used to initialize the bswi, only upon requirement
	const blob_store_tuple_defs* bstd_p;
	const page_access_methods* pam_p;
	const page_modification_methods* pmm_p;
};

// the numeric in context must be EMPTY_DATUM or partially written
digit_write_iterator* get_new_digit_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor pos, uint64_t blob_store_root_page_id, tuple_pointer extension_tail, uint32_t digits_to_be_written_to_prefix, const blob_store_tuple_defs* bstd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p);

void delete_digit_write_iterator(digit_write_iterator* dwi_p, const void* transaction_id, int* abort_error);

uint32_t append_to_digit_write_iterator(digit_write_iterator* dwi_p, const uint64_t* digits, uint32_t digits_size, const heap_table_notifier* notify_wrong_entry, const void* transaction_id, int* abort_error);

#endif