#include<text_blob_read_iterator.h>

#include<page_access_methods.h>

#include<stdlib.h>

text_blob_read_iterator* get_new_text_blob_read_iterator(void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p)
{
	text_blob_read_iterator* tbri_p = malloc(sizeof(text_blob_read_iterator));
	if(tbri_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	tbri_p->is_short = is_text_short_type_info(dti_p) || is_blob_short_type_info(dti_p);

	if(tbri_p->is_short)
		get_value_from_element_from_tuple(&(tbri_p->prefix), tpl_d, inline_accessor, tupl);
	else
	{
		positional_accessor child_inline_accessor = {.positions_length = 0, .positions = malloc(sizeof(uint32_t) * (inline_accessor.positions_length + 1))};
		if(child_inline_accessor.positions == NULL)
			exit(-1);
		append_positions(&child_inline_accessor, inline_accessor);

		append_positions(&child_inline_accessor, STATIC_POSITION(0));
		get_value_from_element_from_tuple(&(tbri_p->prefix), tpl_d, child_inline_accessor, tupl);
		child_inline_accessor.positions_length--;

		user_value worm_head_page_id;
		append_positions(&child_inline_accessor, STATIC_POSITION(1));
		get_value_from_element_from_tuple(&worm_head_page_id, tpl_d, child_inline_accessor, tupl);
		child_inline_accessor.positions_length--;
		tbri_p->extension_head_page_id = worm_head_page_id.uint_value;

		free(child_inline_accessor.positions);
	}

	tbri_p->bytes_read_from_prefix = 0;

	tbri_p->wri_p = NULL;

	tbri_p->wtd_p = wtd_p;
	tbri_p->pam_p = pam_p;

	return tbri_p;
}

void delete_text_blob_read_iterator(text_blob_read_iterator* tbri_p, const void* transaction_id, int* abort_error)
{
	if(tbri_p->wri_p != NULL)
		delete_worm_read_iterator(tbri_p->wri_p, transaction_id, abort_error);
	free(tbri_p);
}

text_blob_read_iterator* clone_text_blob_read_iterator(text_blob_read_iterator* tbri_p, const void* transaction_id, int* abort_error)
{
	text_blob_read_iterator* clone_p = malloc(sizeof(text_blob_read_iterator));
	if(clone_p == NULL)
		exit(-1);

	(*clone_p) = (*tbri_p);

	if((!clone_p->is_short) && (tbri_p->wri_p != NULL)) // if is_large && wri_p != NULL then a wri_p clone is necessary
	{
		clone_p->wri_p = clone_worm_read_iterator(tbri_p->wri_p, transaction_id, abort_error);
		if(*abort_error)
		{
			free(clone_p);
			return NULL;
		}
	}

	return clone_p;
}

uint32_t read_from_text_blob(text_blob_read_iterator* tbri_p, char* data, uint32_t data_size, const void* transaction_id, int* abort_error)
{
	if(data_size == 0)
		return 0;

	uint32_t bytes_read = 0;

	while(data_size > 0)
	{
		uint32_t bytes_read_this_iteration = 0;

		if(tbri_p->bytes_read_from_prefix < tbri_p->prefix.string_or_blob_size) // read from prefix until it is not completely read
		{
			bytes_read_this_iteration = min(data_size, tbri_p->prefix.string_or_blob_size - tbri_p->bytes_read_from_prefix);
			if(data != NULL)
				memory_move(data, tbri_p->prefix.string_or_blob_value + tbri_p->bytes_read_from_prefix, bytes_read_this_iteration);
			tbri_p->bytes_read_from_prefix += bytes_read_this_iteration;
		}
		else if(!(tbri_p->is_short)) // go here only if it is a large text or blob
		{
			// initialize worm read iterator if not done already
			if(tbri_p->wri_p == NULL)
			{
				if(tbri_p->extension_head_page_id == tbri_p->pam_p->pas.NULL_PAGE_ID)
					goto NOTHING_TO_READ;

				tbri_p->wri_p = get_new_worm_read_iterator(tbri_p->extension_head_page_id, tbri_p->wtd_p, tbri_p->pam_p, transaction_id, abort_error);
				if(*abort_error) // on abort error, do nothing
				{
					tbri_p->wri_p = NULL;
					return 0;
				}
			}

			bytes_read_this_iteration = read_from_worm(tbri_p->wri_p, data, data_size, transaction_id, abort_error);
			if(*abort_error) // on abort error, delete worm iterator and set it to NULL
			{
				delete_worm_read_iterator(tbri_p->wri_p, transaction_id, abort_error);
				tbri_p->wri_p = NULL;
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