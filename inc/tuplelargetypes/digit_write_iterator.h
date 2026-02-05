#ifndef DIGIT_WRITE_ITERATOR_H
#define DIGIT_WRITE_ITERATOR_H

#include<tuplestore/tuple.h>
#include<tupleindexer/worm/worm.h>
#include<tuplelargetypes/common_extended.h>

/*
	To be used solely with data_type_info-s that are structurally similar to
	inline types like : numeric_inline,
	extended types like : numeric_extended,

	Note: there should not exist another digit_write_iterator on the same attribute of the same tuple at the same time
*/

typedef struct digit_write_iterator digit_write_iterator;
struct digit_write_iterator
{
	int is_inline:1;

	// shallow copy attributes
	void* tupl;
	const tuple_def* tpl_d;
	positional_accessor inline_accessor;

	uint32_t digits_to_be_written_to_prefix;
	uint32_t digits_written_to_prefix;
	// no more appending to prefix if the above 2 attributes become equal

	// unused if is_inline is set
	worm_append_iterator* wai_p;

	// below attributes only to be used to initialize the wai, only upon requirement
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
	const page_modification_methods* pmm_p;
};

/*
	prefix is appended to a non-persistent tuple, making it as much larger as possible (also controller using digits_to_be_written_to_prefix parameter)
	suffix is transaction protected, and could be persistent
*/

// the numeric in context must be EMPTY_DATUM or partially written
digit_write_iterator* get_new_digit_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor, uint32_t digits_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p);

void delete_digit_write_iterator(digit_write_iterator* dwi_p, const void* transaction_id, int* abort_error);

uint32_t append_to_digit_write_iterator(digit_write_iterator* dwi_p, const uint64_t* digits, uint32_t digits_size, const void* transaction_id, int* abort_error);

#endif