#ifndef DIGIT_READ_ITERATOR_H
#define DIGIT_READ_ITERATOR_H

#include<tuplestore/tuple.h>
#include<tupleindexer/worm/worm.h>
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
	datum inline_digits; // read calls will succeed only if this datum is not null
	uint32_t digits_inline_count;
	uint32_t digits_inline_read; // starts with 0 and increases until digits_inline_count

	uint64_t extension_head_page_id;
	worm_read_iterator* wri_p;

	// below attributes only to be used to initialize the wri, only upon requirement
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
};

digit_read_iterator* get_new_digit_read_iterator(const datum* uval, const data_type_info* dti, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p);

void delete_digit_read_iterator(digit_read_iterator* dri_p, const void* transaction_id, int* abort_error);

uint32_t read_from_digit_read_iterator(digit_read_iterator* dri_p, uint64_t* digits, uint32_t digits_size, const void* transaction_id, int* abort_error);

digit_read_iterator* clone_digit_read_iterator(const digit_read_iterator* dri_p, const void* transaction_id, int* abort_error);

#endif