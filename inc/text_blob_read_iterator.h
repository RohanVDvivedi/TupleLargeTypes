#ifndef TEXT_BLOB_READ_ITERATOR_H
#define TEXT_BLOB_READ_ITERATOR_H

#include<tuple.h>
#include<worm.h>
#include<text_large.h>
#include<blob_large.h>

typedef struct text_blob_read_iterator text_blob_read_iterator;
struct text_blob_read_iterator
{
	int is_short:1;

	// local copy of the prefix and the bytes that have been read from it already
	user_value prefix;
	uint32_t bytes_read_from_prefix;

	// head_page_id to start the worm, unused if is_short is set
	uint64_t extension_head_page_id;

	// above three attributes are cached in the constructor

	// unused if is_short is set
	worm_read_iterator* wri_p;

	// below attributes only to be used to initialize the wri, only upon requirement
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
};

text_blob_read_iterator* get_new_text_blob_read_iterator(const void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p);

void delete_text_blob_read_iterator(text_blob_read_iterator* tbri_p, const void* transaction_id, int* abort_error);

uint32_t read_from_text_blob(text_blob_read_iterator* tbri_p, char* data, uint32_t data_size, const void* transaction_id, int* abort_error);

text_blob_read_iterator* clone_text_blob_read_iterator(text_blob_read_iterator* tbri_p, const void* transaction_id, int* abort_error);

#endif