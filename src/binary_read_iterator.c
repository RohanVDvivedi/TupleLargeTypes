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
	if(data_size == 0)
		return 0;

	// cache the prefix and the extension_head_page_id in local variables
	datum prefix = (*EMPTY_DATUM);
	uint64_t extension_head_page_id = bri_p->pam_p->pas.NULL_PAGE_ID;
	{
		relative_positional_accessor child_relative_accessor;
		initialize_relative_positional_accessor(&child_relative_accessor, &(bri_p->inline_accessor), 1 * (!(bri_p->is_inline)));

		if(bri_p->is_inline)
		{
			point_to_prefix(&child_relative_accessor, bri_p->is_inline);
			int valid_prefix = get_value_from_element_from_tuple(&prefix, bri_p->tpl_d, child_relative_accessor.exact, bri_p->tupl);
			if((!valid_prefix) || is_datum_NULL(&prefix)) // this means it is an uninitialized large text or binary, so treat it as if it is empty, with no worm following it
				prefix = (*EMPTY_DATUM);
		}
		else
		{
			point_to_prefix(&child_relative_accessor, bri_p->is_inline);
			int valid_prefix = get_value_from_element_from_tuple(&prefix, bri_p->tpl_d, child_relative_accessor.exact, bri_p->tupl);
			if((!valid_prefix) || is_datum_NULL(&prefix)) // this means it is an uninitialized large text or binary, so treat it as if it is empty, with no worm following it
			{
				prefix = (*EMPTY_DATUM);
				extension_head_page_id = bri_p->pam_p->pas.NULL_PAGE_ID;
			}
			else // else you need to read the extension_head_page_id
			{
				if(bri_p->wri_p == NULL) // you will need extension_head_page_id set to valid value only if the worm_read_iterator is not initialized
				{
					datum worm_head_page_id;
					point_to_extension_head_page_id(&child_relative_accessor, bri_p->is_inline);
					int valid_worm_head_page_id = get_value_from_element_from_tuple(&worm_head_page_id, bri_p->tpl_d, child_relative_accessor.exact, bri_p->tupl);
					if(valid_worm_head_page_id && !is_datum_NULL(&worm_head_page_id)) // if valid and not NULL
						extension_head_page_id = worm_head_page_id.uint_value;
				}
			}
		}

		deinitialize_relative_positional_accessor(&child_relative_accessor);
	}

	uint32_t bytes_read = 0;

	while(data_size > 0)
	{
		uint32_t bytes_read_this_iteration = 0;

		if(bri_p->bytes_read_from_prefix < prefix.string_or_binary_size) // read from prefix until it is not completely read
		{
			bytes_read_this_iteration = min(data_size, prefix.string_or_binary_size - bri_p->bytes_read_from_prefix);
			if(data != NULL)
				memory_move(data, prefix.string_or_binary_value + bri_p->bytes_read_from_prefix, bytes_read_this_iteration);
			bri_p->bytes_read_from_prefix += bytes_read_this_iteration;
		}
		else if(!(bri_p->is_inline)) // go here only if it is a extended text or binary
		{
			// initialize worm read iterator if not done already
			if(bri_p->wri_p == NULL)
			{
				if(extension_head_page_id == bri_p->pam_p->pas.NULL_PAGE_ID)
					goto NOTHING_TO_READ;

				bri_p->wri_p = get_new_worm_read_iterator(extension_head_page_id, bri_p->wtd_p, bri_p->pam_p, transaction_id, abort_error);
				if(*abort_error) // on abort error, do nothing
				{
					bri_p->wri_p = NULL;
					return 0;
				}
			}

			bytes_read_this_iteration = read_from_worm(bri_p->wri_p, data, data_size, transaction_id, abort_error);
			if(*abort_error) // on abort error, delete worm iterator and set it to NULL
			{
				delete_worm_read_iterator(bri_p->wri_p, transaction_id, abort_error);
				bri_p->wri_p = NULL;
				return 0;
			}
		}

		// skip label to goto, if nothing to be read is found
		NOTHING_TO_READ:;

		if(bytes_read_this_iteration == 0)
			break;

		if(data != NULL)
			data += bytes_read_this_iteration;
		data_size -= bytes_read_this_iteration;
		bytes_read += bytes_read_this_iteration;
	}

	return bytes_read;
}

const char* peek_in_binary_read_iterator(binary_read_iterator* bri_p, uint32_t* data_size, const void* transaction_id, int* abort_error)
{
	if(bri_p->is_null)
		return NULL;

	if(is_empty_dstring(&(bri_p->curr_chunk)))
	{
		if(bri_p->extension_head_page_id == bri_p->pam_p->pas.NULL_PAGE_ID)
			break;

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