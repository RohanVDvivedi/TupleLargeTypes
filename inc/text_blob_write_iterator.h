#ifndef TEXT_BLOB_WRITE_ITERATOR_H
#define TEXT_BLOB_WRITE_ITERATOR_H

#include<tuple.h>
#include<worm.h>
#include<text_large.h>
#include<blob_large.h>

typedef struct text_blob_write_iterator text_blob_write_iterator;
struct text_blob_write_iterator
{
	int is_short:1;

	// shallow copy attributes
	void* tupl;
	tuple_def* tpl_d;
	positional_accessor inline_accessor;

	uint32_t bytes_to_be_written_to_prefix;
	uint32_t bytes_written_to_prefix;
	// no more appending to prefix if the above 2 attributes become equal

	// unused if is_short is set
	worm_append_iterator* wai;

	// below attributes only to be used to initialize the wai, only upon requirement
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
	const page_modification_methods* pmm_p;
};

/*
	prefix is appended to a non-persistent tupl as much long as possible
	suffix is transaction protected, and could be persistent
*/

text_blob_write_iterator* get_new_text_blob_write_iterator(void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor, uint32_t bytes_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p);

void delete_text_blob_write_iterator(text_blob_write_iterator* tbwi_p, const void* transaction_id, int* abort_error);

uint32_t append_to_text_blob(text_blob_write_iterator* tbwi_p, const char* data, uint32_t data_size, const void* transaction_id, int* abort_error);

#endif