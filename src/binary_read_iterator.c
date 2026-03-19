#include<tuplelargetypes/binary_read_iterator.h>

#include<tupleindexer/interface/page_access_methods.h>

#include<stdlib.h>

#include<tuplelargetypes/relative_positional_accessor.h>

binary_read_iterator* get_new_binary_read_iterator(const datum* uval, const data_type_info* dti, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p)
{
	binary_read_iterator* bri_p = malloc(sizeof(binary_read_iterator));
	if(bri_p == NULL)
		exit(-1);

	bri_p->is_null = is_datum_NULL(uval);

	bri_p->curr_chunk = get_dstring_pointing_to(NULL, 0);
	bri_p->extension_head_page_id = pam_p->pas.NULL_PAGE_ID;

	if(!(bri_p->is_null))
	{
		if(dti != NULL && is_extended_type_info(dti))
		{
			{
				datum prefix;
				int valid = get_containee_for_datum(&prefix, uval, dti, EXTENDED_PREFIX_POS_VAL);
				if(valid && !is_datum_NULL(&prefix))
					bri_p->curr_chunk = get_dstring_pointing_to(prefix.string_or_binary_value, prefix.string_or_binary_size);
			}

			bri_p->extension_head_page_id = get_extension_head_page_id_for_extended_type(uval, dti, &(pam_p->pas));
		}
		else
			bri_p->curr_chunk = get_dstring_pointing_to(uval->string_or_binary_value, uval->string_or_binary_size);
	}

	bri_p->wri_p = NULL;

	bri_p->wtd_p = wtd_p;
	bri_p->pam_p = pam_p;

	return bri_p;
}

void delete_binary_read_iterator(binary_read_iterator* bri_p, const void* transaction_id, int* abort_error)
{
	if(bri_p->wri_p != NULL)
		delete_worm_read_iterator(bri_p->wri_p, transaction_id, abort_error);
	free(bri_p);
}

binary_read_iterator* clone_binary_read_iterator(const binary_read_iterator* bri_p, const void* transaction_id, int* abort_error)
{
	binary_read_iterator* clone_p = malloc(sizeof(binary_read_iterator));
	if(clone_p == NULL)
		exit(-1);

	(*clone_p) = (*bri_p);

	if((bri_p->wri_p != NULL))
	{
		clone_p->wri_p = clone_worm_read_iterator(bri_p->wri_p, transaction_id, abort_error);
		if(*abort_error)
		{
			free(clone_p);
			return NULL;
		}
	}

	return clone_p;
}

uint32_t read_from_binary_read_iterator(binary_read_iterator* bri_p, char* data, uint32_t data_size, const void* transaction_id, int* abort_error)
{
	if(bri_p->is_null)
		return 0;

	uint32_t bytes_read = 0;

	while(bytes_read < data_size)
	{
		// if the current chunk is empty make it peek
		if(is_empty_dstring(&(bri_p->curr_chunk)))
		{
			uint32_t peeked_bytes;
			peek_in_binary_read_iterator(bri_p, &peeked_bytes, transaction_id, abort_error);
			if(*abort_error)
				return 0;
			if(peeked_bytes == 0) // if nothing could be peeked, then we are at the end
				break;
		}

		// evaluate the bytes you plan to read in this iteration
		uint32_t bytes_read_this_iteration = min(data_size - bytes_read, get_char_count_dstring(&(bri_p->curr_chunk)));

		// copy from current chunk, only if data is not empty
		if(data)
			memory_move(data + bytes_read, get_byte_array_dstring(&(bri_p->curr_chunk)), bytes_read_this_iteration);

		// consume bytes from both the current chunk and the wri_p, if it is not null
		discard_chars_from_front_dstring(&(bri_p->curr_chunk), bytes_read_this_iteration);
		if(bri_p->wri_p != NULL)
		{
			read_from_worm(bri_p->wri_p, NULL, bytes_read_this_iteration, transaction_id, abort_error);
			if(*abort_error)
				return 0;
		}

		bytes_read += bytes_read_this_iteration;
	}

	return bytes_read;
}

const char* peek_in_binary_read_iterator(binary_read_iterator* bri_p, uint32_t* data_size, const void* transaction_id, int* abort_error)
{
	if(bri_p->is_null)
		return NULL;

	// we may need to peek in the work, only if the current chunk is empty and the extension_head_page_id exists
	if(is_empty_dstring(&(bri_p->curr_chunk)) && (bri_p->extension_head_page_id != bri_p->pam_p->pas.NULL_PAGE_ID))
	{
		if(bri_p->wri_p == NULL)
		{
			bri_p->wri_p = get_new_worm_read_iterator(bri_p->extension_head_page_id, bri_p->wtd_p, bri_p->pam_p, transaction_id, abort_error);
			if(*abort_error)
				return NULL;
		}

		uint32_t data_size = 0;
		void* data = peek_in_worm(bri_p->wri_p, &data_size, transaction_id, abort_error);
		if(*abort_error)
			return NULL;

		bri_p->curr_chunk = get_dstring_pointing_to(data, data_size);
	}

	(*data_size) = min(get_char_count_dstring(&(bri_p->curr_chunk)), UINT32_MAX);
	return get_byte_array_dstring(&(bri_p->curr_chunk));
}