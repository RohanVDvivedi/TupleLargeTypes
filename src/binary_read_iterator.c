#include<tuplelargetypes/binary_read_iterator.h>

#include<tupleindexer/interface/page_access_methods.h>

#include<stdlib.h>

#include<tuplelargetypes/relative_positional_accessor.h>

#include<tuplelargetypes/binary_iterator_commons.h>

binary_read_iterator* get_new_binary_read_iterator(const void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p)
{
	binary_read_iterator* bri_p = malloc(sizeof(binary_read_iterator));
	if(bri_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	bri_p->is_inline = is_inline_type_info(dti_p);

	bri_p->tupl = tupl;
	bri_p->tpl_d = tpl_d;
	bri_p->inline_accessor = inline_accessor;

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

uint32_t read_from_binary_read_iterator(binary_read_iterator* bri_p, char* data, uint32_t data_size, const void* transaction_id, int* abort_error)
{
	if(data_size == 0)
		return 0;

	// cache the prefix and the extension_head_page_id in local variables
	user_value prefix = (*EMPTY_USER_VALUE);
	uint64_t extension_head_page_id = bri_p->pam_p->pas.NULL_PAGE_ID;
	{
		relative_positional_accessor child_relative_accessor;
		initialize_relative_positional_accessor(&child_relative_accessor, &(bri_p->inline_accessor), 1 * (!(bri_p->is_inline)));

		if(bri_p->is_inline)
		{
			point_to_prefix(&child_relative_accessor, bri_p->is_inline);
			int valid_prefix = get_value_from_element_from_tuple(&prefix, bri_p->tpl_d, child_relative_accessor.exact, bri_p->tupl);
			if((!valid_prefix) || is_user_value_NULL(&prefix)) // this means it is an uninitialized large text or blob, so treat it as if it is empty, with no worm following it
				prefix = (*EMPTY_USER_VALUE);
		}
		else
		{
			point_to_prefix(&child_relative_accessor, bri_p->is_inline);
			int valid_prefix = get_value_from_element_from_tuple(&prefix, bri_p->tpl_d, child_relative_accessor.exact, bri_p->tupl);
			if((!valid_prefix) || is_user_value_NULL(&prefix)) // this means it is an uninitialized large text or blob, so treat it as if it is empty, with no worm following it
			{
				prefix = (*EMPTY_USER_VALUE);
				extension_head_page_id = bri_p->pam_p->pas.NULL_PAGE_ID;
			}
			else // else you need to read the extension_head_page_id
			{
				if(bri_p->wri_p == NULL) // you will need extension_head_page_id set to valid value only if the worm_read_iterator is not initialized
				{
					user_value worm_head_page_id;
					point_to_extension_head_page_id(&child_relative_accessor, bri_p->is_inline);
					int valid_worm_head_page_id = get_value_from_element_from_tuple(&worm_head_page_id, bri_p->tpl_d, child_relative_accessor.exact, bri_p->tupl);
					if(valid_worm_head_page_id && !is_user_value_NULL(&worm_head_page_id)) // if valid and not NULL
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

		if(bri_p->bytes_read_from_prefix < prefix.string_or_blob_size) // read from prefix until it is not completely read
		{
			bytes_read_this_iteration = min(data_size, prefix.string_or_blob_size - bri_p->bytes_read_from_prefix);
			if(data != NULL)
				memory_move(data, prefix.string_or_blob_value + bri_p->bytes_read_from_prefix, bytes_read_this_iteration);
			bri_p->bytes_read_from_prefix += bytes_read_this_iteration;
		}
		else if(!(bri_p->is_inline)) // go here only if it is a large text or blob
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