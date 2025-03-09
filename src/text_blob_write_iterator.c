#include<text_blob_write_iterator.h>

#include<stdlib.h>
#include<page_access_methods.h>

#include<relative_positional_accessor.h>

static inline void point_to_attribute(relative_positional_accessor* rpa, int is_short)
{
	relative_positonal_accessor_set_from_relative(rpa, SELF);
}

static inline void point_to_prefix(relative_positional_accessor* rpa, int is_short)
{
	if(is_short)
	{
		relative_positonal_accessor_set_from_relative(rpa, SELF);
		return ;
	}

	relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(0));
}

static inline void point_to_prefix_character(relative_positional_accessor* rpa, uint32_t index, int is_short)
{
	if(is_short)
	{
		relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(index));
		return ;
	}

	relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(0, index));
}

static inline void point_to_extension_head_page_id(relative_positional_accessor* rpa, int is_short)
{
	if(is_short)
		return ;

	relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(1));
}

text_blob_write_iterator* get_new_text_blob_write_iterator(void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor, uint32_t bytes_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p)
{
	text_blob_write_iterator* tbwi_p = malloc(sizeof(text_blob_write_iterator));
	if(tbwi_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	tbwi_p->is_short = is_text_short_type_info(dti_p) || is_blob_short_type_info(dti_p);

	tbwi_p->tupl = tupl;
	tbwi_p->tpl_d = tpl_d;
	tbwi_p->inline_accessor = inline_accessor;

	// if it is a large_string or large_blob, with a worm attached then make bytes_to_be_written_to_prefix = 0
	if(!(tbwi_p->is_short))
	{
		relative_positional_accessor child_relative_accessor;
		initialize_relative_positional_accessor(&child_relative_accessor, &(tbwi_p->inline_accessor), 2);

		point_to_extension_head_page_id(&child_relative_accessor, tbwi_p->is_short);
		user_value extension_head_page;
		int valid_extension = get_value_from_element_from_tuple(&extension_head_page, tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl);

		point_to_prefix(&child_relative_accessor, tbwi_p->is_short);
		user_value prefix;
		int valid_prefix = get_value_from_element_from_tuple(&prefix, tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl);

		if(valid_extension && valid_prefix && !is_user_value_NULL(&extension_head_page) && !is_user_value_NULL(&prefix) && extension_head_page.uint_value != pam_p->pas.NULL_PAGE_ID)
			bytes_to_be_written_to_prefix = 0;

		deinitialize_relative_positional_accessor(&child_relative_accessor);
	}

	tbwi_p->bytes_to_be_written_to_prefix = bytes_to_be_written_to_prefix;
	tbwi_p->bytes_written_to_prefix = 0;

	tbwi_p->wai_p = NULL;

	tbwi_p->wtd_p = wtd_p;
	tbwi_p->pam_p = pam_p;
	tbwi_p->pmm_p = pmm_p;

	return tbwi_p;
}

void delete_text_blob_write_iterator(text_blob_write_iterator* tbwi_p, const void* transaction_id, int* abort_error)
{
	if(tbwi_p->wai_p != NULL)
		delete_worm_append_iterator(tbwi_p->wai_p, transaction_id, abort_error);
	free(tbwi_p);
}

uint32_t append_to_text_blob(text_blob_write_iterator* tbwi_p, const char* data, uint32_t data_size, const void* transaction_id, int* abort_error)
{
	if(data_size == 0)
		return 0;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(tbwi_p->inline_accessor), 2);

	// initialization
	// if the attribute is NULL, set it to EMPTY_USER_VALUE
	{
		point_to_attribute(&child_relative_accessor, tbwi_p->is_short);
		user_value attr;
		get_value_from_element_from_tuple(&attr, tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl);
		if(is_user_value_NULL(&attr))
			set_element_in_tuple(tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl, EMPTY_USER_VALUE, UINT32_MAX);
	}

	// if the prefix is NULL in a large text or blob, set it to EMPTY_USER_VALUE and then bytes_to_be_written_to_prefix = min(bytes_to_be_written_to_prefix, max_size_increment_allowed);, then set the blob_extension to NULL_PAGE_ID
	if(!(tbwi_p->is_short))
	{
		point_to_prefix(&child_relative_accessor, tbwi_p->is_short);
		user_value prefix;
		get_value_from_element_from_tuple(&prefix, tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl);
		int reset = 0;
		if(is_user_value_NULL(&prefix))
		{
			reset = 1;
			set_element_in_tuple(tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl, EMPTY_USER_VALUE, UINT32_MAX);
		}
		tbwi_p->bytes_to_be_written_to_prefix = min(tbwi_p->bytes_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl));

		if(reset)
		{
			point_to_extension_head_page_id(&child_relative_accessor, tbwi_p->is_short);
			set_element_in_tuple(tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl, &((user_value){.uint_value = tbwi_p->pam_p->pas.NULL_PAGE_ID}), UINT32_MAX);
		}
	}
	else
	{
		point_to_attribute(&child_relative_accessor, tbwi_p->is_short);
		tbwi_p->bytes_to_be_written_to_prefix = min(tbwi_p->bytes_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl));
	}

	uint32_t bytes_written = 0;

	while(data_size > 0)
	{
		uint32_t bytes_written_this_iteration = 0;

		if(tbwi_p->bytes_to_be_written_to_prefix > tbwi_p->bytes_written_to_prefix)
		{
			bytes_written_this_iteration = min(data_size, tbwi_p->bytes_to_be_written_to_prefix - tbwi_p->bytes_written_to_prefix);

			// grab old_element_count and expand the container
			point_to_prefix(&child_relative_accessor, tbwi_p->is_short);
			uint32_t old_element_count = get_element_count_for_element_from_tuple(tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl);
			expand_element_count_for_element_in_tuple(tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl, old_element_count, bytes_written_this_iteration, bytes_written_this_iteration);
			
			// copy data into it byte by byte
			for(uint32_t i = 0; i < bytes_written_this_iteration; i++)
			{
				point_to_prefix_character(&child_relative_accessor, old_element_count + i, tbwi_p->is_short);
				set_element_in_tuple(tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl, &((user_value){.uint_value = data[i]}), UINT32_MAX);
			}

			tbwi_p->bytes_written_to_prefix += bytes_written_this_iteration;
		}
		else if(!(tbwi_p->is_short))
		{
			if(tbwi_p->wai_p == NULL)
			{
				// read extension_head_page_id
				uint64_t extension_head_page_id;
				{
					point_to_extension_head_page_id(&child_relative_accessor, tbwi_p->is_short);
					user_value extension_head;
					get_value_from_element_from_tuple(&extension_head, tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl);
					extension_head_page_id = extension_head.uint_value;
				}

				// if it is NULL_PAGE_ID, then create a new worm and set it in the attribute beside prefix
				if(extension_head_page_id == tbwi_p->pam_p->pas.NULL_PAGE_ID)
				{
					extension_head_page_id = get_new_worm(1, tbwi_p->pam_p->pas.NULL_PAGE_ID, tbwi_p->wtd_p, tbwi_p->pam_p, tbwi_p->pmm_p, transaction_id, abort_error);
					if(*abort_error)
					{
						deinitialize_relative_positional_accessor(&child_relative_accessor);
						return 0;
					}
					set_element_in_tuple(tbwi_p->tpl_d, child_relative_accessor.exact, tbwi_p->tupl, &((user_value){.uint_value = extension_head_page_id}), UINT32_MAX);
				}

				// open a new wai
				tbwi_p->wai_p = get_new_worm_append_iterator(extension_head_page_id, tbwi_p->wtd_p, tbwi_p->pam_p, tbwi_p->pmm_p, transaction_id, abort_error);
				if(*abort_error)
				{
					tbwi_p->wai_p = NULL;
					deinitialize_relative_positional_accessor(&child_relative_accessor);
					return 0;
				}
			}

			// append to worm
			bytes_written_this_iteration = append_to_worm(tbwi_p->wai_p, data, data_size, transaction_id, abort_error);
			if(*abort_error)
			{
				delete_worm_append_iterator(tbwi_p->wai_p, transaction_id, abort_error);
				tbwi_p->wai_p = NULL;
				deinitialize_relative_positional_accessor(&child_relative_accessor);
				return 0;
			}
		}

		if(bytes_written_this_iteration == 0)
			break;

		data += bytes_written_this_iteration;
		data_size -= bytes_written_this_iteration;
		bytes_written += bytes_written_this_iteration;
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);
	return bytes_written;
}