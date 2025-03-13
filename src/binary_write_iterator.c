#include<binary_write_iterator.h>

#include<page_access_methods.h>

#include<stdlib.h>

#include<relative_positional_accessor.h>

#include<binary_iterator_commons.h>

binary_write_iterator* get_new_binary_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor, uint32_t bytes_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p)
{
	binary_write_iterator* bwi_p = malloc(sizeof(binary_write_iterator));
	if(bwi_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	bwi_p->is_inline = is_inline_type_info(dti_p);

	bwi_p->tupl = tupl;
	bwi_p->tpl_d = tpl_d;
	bwi_p->inline_accessor = inline_accessor;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(bwi_p->inline_accessor),2);

	// if it is a large_string or large_blob, with a worm attached then make bytes_to_be_written_to_prefix = 0
	if(!(bwi_p->is_inline))
	{
		point_to_extension_head_page_id(&child_relative_accessor, bwi_p->is_inline);
		user_value extension_head_page;
		int valid_extension = get_value_from_element_from_tuple(&extension_head_page, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);

		point_to_prefix(&child_relative_accessor, bwi_p->is_inline);
		user_value prefix;
		int valid_prefix = get_value_from_element_from_tuple(&prefix, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);

		if(valid_extension && valid_prefix && !is_user_value_NULL(&extension_head_page) && !is_user_value_NULL(&prefix) && extension_head_page.uint_value != pam_p->pas.NULL_PAGE_ID)
			bytes_to_be_written_to_prefix = 0;
	}

	bwi_p->bytes_to_be_written_to_prefix = bytes_to_be_written_to_prefix;
	bwi_p->bytes_written_to_prefix = 0;

	bwi_p->wai_p = NULL;

	bwi_p->wtd_p = wtd_p;
	bwi_p->pam_p = pam_p;
	bwi_p->pmm_p = pmm_p;

	// initialization
	// if the attribute is NULL, set it to EMPTY_USER_VALUE
	{
		point_to_attribute(&child_relative_accessor, bwi_p->is_inline);
		user_value attr;
		int valid_attr = get_value_from_element_from_tuple(&attr, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);
		if(!valid_attr)
		{
			free(bwi_p);
			deinitialize_relative_positional_accessor(&child_relative_accessor);
			return NULL;
		}
		if(is_user_value_NULL(&attr))
			set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, EMPTY_USER_VALUE, UINT32_MAX);
	}

	// if the prefix is NULL in a large text or blob, set it to EMPTY_USER_VALUE and then bytes_to_be_written_to_prefix = min(bytes_to_be_written_to_prefix, max_size_increment_allowed);, then set the blob_extension to NULL_PAGE_ID
	if(!(bwi_p->is_inline))
	{
		point_to_prefix(&child_relative_accessor, bwi_p->is_inline);
		user_value prefix;
		get_value_from_element_from_tuple(&prefix, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);
		int reset = 0;
		if(is_user_value_NULL(&prefix))
		{
			reset = 1;
			set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, EMPTY_USER_VALUE, UINT32_MAX);
		}
		bwi_p->bytes_to_be_written_to_prefix = min(bwi_p->bytes_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl));

		if(reset)
		{
			point_to_extension_head_page_id(&child_relative_accessor, bwi_p->is_inline);
			set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, &((user_value){.uint_value = bwi_p->pam_p->pas.NULL_PAGE_ID}), UINT32_MAX);
		}
	}
	else
	{
		point_to_attribute(&child_relative_accessor, bwi_p->is_inline);
		bwi_p->bytes_to_be_written_to_prefix = min(bwi_p->bytes_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl));
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);
	return bwi_p;
}

void delete_binary_write_iterator(binary_write_iterator* bwi_p, const void* transaction_id, int* abort_error)
{
	if(bwi_p->wai_p != NULL)
		delete_worm_append_iterator(bwi_p->wai_p, transaction_id, abort_error);
	free(bwi_p);
}

uint32_t append_to_binary_write_iterator(binary_write_iterator* bwi_p, const char* data, uint32_t data_size, const void* transaction_id, int* abort_error)
{
	if(data_size == 0)
		return 0;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(bwi_p->inline_accessor), 2);

	uint32_t bytes_written = 0;

	while(data_size > 0)
	{
		uint32_t bytes_written_this_iteration = 0;

		if(bwi_p->bytes_to_be_written_to_prefix > bwi_p->bytes_written_to_prefix)
		{
			bytes_written_this_iteration = min(data_size, bwi_p->bytes_to_be_written_to_prefix - bwi_p->bytes_written_to_prefix);

			// grab old_element_count and expand the container
			point_to_prefix(&child_relative_accessor, bwi_p->is_inline);
			uint32_t old_element_count = get_element_count_for_element_from_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);
			expand_element_count_for_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, old_element_count, bytes_written_this_iteration, bytes_written_this_iteration);
			
			// copy data into it byte by byte
			for(uint32_t i = 0; i < bytes_written_this_iteration; i++)
			{
				point_to_prefix_s_byte(&child_relative_accessor, old_element_count + i, bwi_p->is_inline);
				set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, &((user_value){.uint_value = data[i]}), UINT32_MAX);
			}

			bwi_p->bytes_written_to_prefix += bytes_written_this_iteration;
		}
		else if(!(bwi_p->is_inline))
		{
			if(bwi_p->wai_p == NULL)
			{
				// read extension_head_page_id
				uint64_t extension_head_page_id;
				{
					point_to_extension_head_page_id(&child_relative_accessor, bwi_p->is_inline);
					user_value extension_head;
					get_value_from_element_from_tuple(&extension_head, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);
					extension_head_page_id = extension_head.uint_value;
				}

				// if it is NULL_PAGE_ID, then create a new worm and set it in the attribute beside prefix
				if(extension_head_page_id == bwi_p->pam_p->pas.NULL_PAGE_ID)
				{
					extension_head_page_id = get_new_worm(1, bwi_p->pam_p->pas.NULL_PAGE_ID, bwi_p->wtd_p, bwi_p->pam_p, bwi_p->pmm_p, transaction_id, abort_error);
					if(*abort_error)
					{
						deinitialize_relative_positional_accessor(&child_relative_accessor);
						return 0;
					}
					set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, &((user_value){.uint_value = extension_head_page_id}), UINT32_MAX);
				}

				// open a new wai
				bwi_p->wai_p = get_new_worm_append_iterator(extension_head_page_id, bwi_p->wtd_p, bwi_p->pam_p, bwi_p->pmm_p, transaction_id, abort_error);
				if(*abort_error)
				{
					bwi_p->wai_p = NULL;
					deinitialize_relative_positional_accessor(&child_relative_accessor);
					return 0;
				}
			}

			// append to worm
			bytes_written_this_iteration = append_to_worm(bwi_p->wai_p, data, data_size, transaction_id, abort_error);
			if(*abort_error)
			{
				delete_worm_append_iterator(bwi_p->wai_p, transaction_id, abort_error);
				bwi_p->wai_p = NULL;
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