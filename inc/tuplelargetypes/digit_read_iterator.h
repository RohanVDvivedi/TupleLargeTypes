#ifndef DIGIT_READ_ITERATOR_H
#define DIGIT_READ_ITERATOR_H

#include<tuplestore/tuple.h>
#include<tupleindexer/blob_store/blob_store.h>
#include<tuplelargetypes/common_extended.h>

/*
	To be used solely with data_type_info-s that are structurally similar to
	inline types like : numeric_inline, 
	extended types like : numeric_extended,

	Note: there should not exist another digit_write_iterator on the same attribute of the same tuple at the same time
*/

typedef struct digit_read_iterator digit_read_iterator;
struct digit_read_iterator
{
	unsigned int is_extended:1;

	datum digits_inline; // read calls will succeed only if this datum is not null
	uint32_t digits_inline_count;
	uint32_t digits_inline_read; // starts with 0 and increases until digits_inline_count

	const data_type_info* digits_dti;

	tuple_pointer extension_head;
	blob_store_read_iterator* bsri_p;

	// below attributes only to be used to initialize the bsri, only upon requirement
	const blob_store_tuple_defs* bstd_p;
	const page_access_methods* pam_p;
};

digit_read_iterator* get_new_digit_read_iterator(const datum* uval, const data_type_info* dti, const blob_store_tuple_defs* bstd_p, const page_access_methods* pam_p);

void delete_digit_read_iterator(digit_read_iterator* dri_p, const void* transaction_id, int* abort_error);

uint32_t read_from_digit_read_iterator(digit_read_iterator* dri_p, uint64_t* digits, uint32_t digits_size, const void* transaction_id, int* abort_error);

digit_read_iterator* clone_digit_read_iterator(const digit_read_iterator* dri_p, const void* transaction_id, int* abort_error);

#endif