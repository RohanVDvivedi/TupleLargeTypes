#include<binary_read_iterator.h>

#include<page_access_methods.h>

#include<stdlib.h>

#include<relative_positional_accessor.h>

binary_read_iterator* get_new_binary_read_iterator(const void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p)
{
	binary_read_iterator* bri_p = malloc(sizeof(binary_read_iterator));
	if(bri_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	bri_p->is_inline = is_inline_type_info(dti_p);

	if(bri_p->is_inline)
	{
		int valid_prefix = get_value_from_element_from_tuple(&(bri_p->prefix), tpl_d, inline_accessor, tupl);
		if((!valid_prefix) || is_user_value_NULL(&(bri_p->prefix))) // this means it is an uninitialized large text or blob, so treat it as if it is empty, with no worm following it
			bri_p->prefix.string_or_blob_size = 0;
	}
	else
	{
		relative_positional_accessor child_relative_accessor;
		initialize_relative_positional_accessor(&child_relative_accessor, &inline_accessor, 1);

		relative_positonal_accessor_set_from_relative(&child_relative_accessor, STATIC_POSITION(0));
		int valid_prefix = get_value_from_element_from_tuple(&(bri_p->prefix), tpl_d, child_relative_accessor.exact, tupl);
		if((!valid_prefix) || is_user_value_NULL(&(bri_p->prefix))) // this means it is an uninitialized large text or blob, so treat it as if it is empty, with no worm following it
		{
			bri_p->prefix.string_or_blob_size = 0;
			bri_p->extension_head_page_id = pam_p->pas.NULL_PAGE_ID;
		}
		else // else you need to read the extension_head_page_id
		{
			user_value worm_head_page_id;
			relative_positonal_accessor_set_from_relative(&child_relative_accessor, STATIC_POSITION(1));
			int valid_worm_head_page_id = get_value_from_element_from_tuple(&worm_head_page_id, tpl_d, child_relative_accessor.exact, tupl);
			if((!valid_worm_head_page_id) || is_user_value_NULL(&worm_head_page_id)) // if not valid or NULL, then NULL initialize it
				bri_p->extension_head_page_id = pam_p->pas.NULL_PAGE_ID;
			else
				bri_p->extension_head_page_id = worm_head_page_id.uint_value;
		}

		deinitialize_relative_positional_accessor(&child_relative_accessor);
	}

	bri_p->bytes_read_from_prefix = 0;

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

binary_read_iterator* clone_binary_read_iterator(binary_read_iterator* bri_p, const void* transaction_id, int* abort_error)
{
	binary_read_iterator* clone_p = malloc(sizeof(binary_read_iterator));
	if(clone_p == NULL)
		exit(-1);

	(*clone_p) = (*bri_p);

	if((!clone_p->is_inline) && (bri_p->wri_p != NULL)) // if is_large && wri_p != NULL then a wri_p clone is necessary
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

uint32_t read_from_text_blob(binary_read_iterator* bri_p, char* data, uint32_t data_size, const void* transaction_id, int* abort_error)
{
	if(data_size == 0)
		return 0;

	uint32_t bytes_read = 0;

	while(data_size > 0)
	{
		uint32_t bytes_read_this_iteration = 0;

		if(bri_p->bytes_read_from_prefix < bri_p->prefix.string_or_blob_size) // read from prefix until it is not completely read
		{
			bytes_read_this_iteration = min(data_size, bri_p->prefix.string_or_blob_size - bri_p->bytes_read_from_prefix);
			if(data != NULL)
				memory_move(data, bri_p->prefix.string_or_blob_value + bri_p->bytes_read_from_prefix, bytes_read_this_iteration);
			bri_p->bytes_read_from_prefix += bytes_read_this_iteration;
		}
		else if(!(bri_p->is_inline)) // go here only if it is a large text or blob
		{
			// initialize worm read iterator if not done already
			if(bri_p->wri_p == NULL)
			{
				if(bri_p->extension_head_page_id == bri_p->pam_p->pas.NULL_PAGE_ID)
					goto NOTHING_TO_READ;

				bri_p->wri_p = get_new_worm_read_iterator(bri_p->extension_head_page_id, bri_p->wtd_p, bri_p->pam_p, transaction_id, abort_error);
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

		data += bytes_read_this_iteration;
		data_size -= bytes_read_this_iteration;
		bytes_read += bytes_read_this_iteration;
	}

	return bytes_read;
}