#include<digit_write_iterator.h>

#include<page_access_methods.h>

#include<stdlib.h>

#include<relative_positional_accessor.h>

#include<digit_iterator_commons.h>

digit_write_iterator* get_new_digit_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor, uint32_t digits_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p)
{
	digit_write_iterator* dwi_p = malloc(sizeof(digit_write_iterator));
	if(dwi_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	dwi_p->is_inline = is_inline_type_info(dti_p);

	dwi_p->tupl = tupl;
	dwi_p->tpl_d = tpl_d;
	dwi_p->inline_accessor = inline_accessor;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(dwi_p->inline_accessor),3);

	// if it is a numeric_extended, with a worm attached then make digits_to_be_written_to_prefix = 0
	if(!(dwi_p->is_inline))
	{
		point_to_extension_head_page_id(&child_relative_accessor, dwi_p->is_inline);
		user_value extension_head_page;
		int valid_extension = get_value_from_element_from_tuple(&extension_head_page, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);

		point_to_prefix(&child_relative_accessor, dwi_p->is_inline);
		user_value prefix;
		int valid_prefix = get_value_from_element_from_tuple(&prefix, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);

		if(valid_extension && valid_prefix && !is_user_value_NULL(&extension_head_page) && !is_user_value_NULL(&prefix) && extension_head_page.uint_value != pam_p->pas.NULL_PAGE_ID)
			digits_to_be_written_to_prefix = 0;
	}

	dwi_p->digits_to_be_written_to_prefix = digits_to_be_written_to_prefix;
	dwi_p->digits_written_to_prefix = 0;

	dwi_p->wai_p = NULL;

	dwi_p->wtd_p = wtd_p;
	dwi_p->pam_p = pam_p;
	dwi_p->pmm_p = pmm_p;

	// if the prefix is NULL in an extended numeric, set it to EMPTY_USER_VALUE and then bytes_to_be_written_to_prefix = min(bytes_to_be_written_to_prefix, max_size_increment_allowed);, then set the blob_extension to NULL_PAGE_ID
	if(!(dwi_p->is_inline))
	{
		// if prefix_container is NULL set it to EMPTY_USER_VALUE
		{
			point_to_prefix_container(&child_relative_accessor, dwi_p->is_inline);
			user_value prefix_container;
			get_value_from_element_from_tuple(&prefix_container, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
			if(is_user_value_NULL(&prefix_container))
				set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, EMPTY_USER_VALUE, UINT32_MAX);
		}
		
		// if prefix is NULL set it to EMPTY_USER_VALUE
		point_to_prefix(&child_relative_accessor, dwi_p->is_inline);
		user_value prefix;
		get_value_from_element_from_tuple(&prefix, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
		int reset = 0;
		if(is_user_value_NULL(&prefix))
		{
			reset = 1;
			set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, EMPTY_USER_VALUE, UINT32_MAX);
		}
		dwi_p->digits_to_be_written_to_prefix = min(dwi_p->digits_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl) / 5);

		if(reset)
		{
			point_to_extension_head_page_id(&child_relative_accessor, dwi_p->is_inline);
			set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, &((user_value){.uint_value = dwi_p->pam_p->pas.NULL_PAGE_ID}), UINT32_MAX);
		}
	}
	else
	{
		// if prefix is NULL set it to EMPTY_USER_VALUE
		{
			point_to_prefix(&child_relative_accessor, dwi_p->is_inline);
			user_value prefix;
			get_value_from_element_from_tuple(&prefix, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
			if(is_user_value_NULL(&prefix))
				set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, EMPTY_USER_VALUE, UINT32_MAX);
		}

		// get maximum digits you can extend it additionally by
		dwi_p->digits_to_be_written_to_prefix = min(dwi_p->digits_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl) / 5);
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);
	return dwi_p;
}

void delete_digit_write_iterator(digit_write_iterator* dwi_p, const void* transaction_id, int* abort_error)
{
	if(dwi_p->wai_p != NULL)
		delete_worm_append_iterator(dwi_p->wai_p, transaction_id, abort_error);
	free(dwi_p);
}