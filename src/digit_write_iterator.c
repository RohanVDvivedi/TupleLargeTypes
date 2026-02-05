#include<tuplelargetypes/digit_write_iterator.h>

#include<tupleindexer/interface/page_access_methods.h>

#include<stdlib.h>

#include<tuplelargetypes/relative_positional_accessor.h>

#include<tuplelargetypes/digit_iterator_commons.h>

#include<tuplelargetypes/numeric_extended.h>

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
		datum extension_head_page;
		int valid_extension = get_value_from_element_from_tuple(&extension_head_page, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);

		point_to_prefix(&child_relative_accessor, dwi_p->is_inline);
		datum prefix;
		int valid_prefix = get_value_from_element_from_tuple(&prefix, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);

		if(valid_extension && valid_prefix && !is_datum_NULL(&extension_head_page) && !is_datum_NULL(&prefix) && extension_head_page.uint_value != pam_p->pas.NULL_PAGE_ID)
			digits_to_be_written_to_prefix = 0;
	}

	dwi_p->digits_to_be_written_to_prefix = digits_to_be_written_to_prefix;
	dwi_p->digits_written_to_prefix = 0;

	dwi_p->wai_p = NULL;

	dwi_p->wtd_p = wtd_p;
	dwi_p->pam_p = pam_p;
	dwi_p->pmm_p = pmm_p;

	// if the prefix is NULL in an extended numeric, set it to EMPTY_DATUM and then bytes_to_be_written_to_prefix = min(bytes_to_be_written_to_prefix, max_size_increment_allowed);, then set the binary_extension to NULL_PAGE_ID
	if(!(dwi_p->is_inline))
	{
		// if prefix_container is NULL set it to EMPTY_DATUM
		{
			point_to_prefix_container(&child_relative_accessor, dwi_p->is_inline);
			datum prefix_container;
			get_value_from_element_from_tuple(&prefix_container, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
			if(is_datum_NULL(&prefix_container))
				set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, EMPTY_DATUM, UINT32_MAX);
		}
		
		// if prefix is NULL set it to EMPTY_DATUM
		point_to_prefix(&child_relative_accessor, dwi_p->is_inline);
		datum prefix;
		get_value_from_element_from_tuple(&prefix, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
		int reset = 0;
		if(is_datum_NULL(&prefix))
		{
			reset = 1;
			set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, EMPTY_DATUM, UINT32_MAX);
		}
		dwi_p->digits_to_be_written_to_prefix = min(dwi_p->digits_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl) / BYTES_PER_NUMERIC_DIGIT);

		if(reset)
		{
			point_to_extension_head_page_id(&child_relative_accessor, dwi_p->is_inline);
			set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, &((datum){.uint_value = dwi_p->pam_p->pas.NULL_PAGE_ID}), UINT32_MAX);
		}
	}
	else
	{
		// if prefix is NULL set it to EMPTY_DATUM
		{
			point_to_prefix(&child_relative_accessor, dwi_p->is_inline);
			datum prefix;
			get_value_from_element_from_tuple(&prefix, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
			if(is_datum_NULL(&prefix))
				set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, EMPTY_DATUM, UINT32_MAX);
		}

		// get maximum digits you can extend it additionally by
		dwi_p->digits_to_be_written_to_prefix = min(dwi_p->digits_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl) / BYTES_PER_NUMERIC_DIGIT);
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

uint32_t append_to_digit_write_iterator(digit_write_iterator* dwi_p, const uint64_t* digits, uint32_t digits_size, const void* transaction_id, int* abort_error)
{
	if(digits_size == 0)
		return 0;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(dwi_p->inline_accessor), 3);

	uint32_t digits_written = 0;

	while(digits_size > 0)
	{
		uint32_t digits_written_this_iteration = 0;

		if(dwi_p->digits_to_be_written_to_prefix > dwi_p->digits_written_to_prefix)
		{
			digits_written_this_iteration = min(digits_size, dwi_p->digits_to_be_written_to_prefix - dwi_p->digits_written_to_prefix);

			// grab old_element_count and expand the container
			point_to_prefix(&child_relative_accessor, dwi_p->is_inline);
			uint32_t old_element_count = get_element_count_for_element_from_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
			expand_element_count_for_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, old_element_count, digits_written_this_iteration, digits_written_this_iteration * BYTES_PER_NUMERIC_DIGIT);

			// copy data into it byte by byte
			for(uint32_t i = 0; i < digits_written_this_iteration; i++)
			{
				point_to_prefix_s_digit(&child_relative_accessor, old_element_count + i, dwi_p->is_inline);
				set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, &((datum){.uint_value = digits[i]}), UINT32_MAX);
			}

			dwi_p->digits_written_to_prefix += digits_written_this_iteration;
		}
		else if(!(dwi_p->is_inline))
		{
			if(dwi_p->wai_p == NULL)
			{
				// read extension_head_page_id
				uint64_t extension_head_page_id;
				{
					point_to_extension_head_page_id(&child_relative_accessor, dwi_p->is_inline);
					datum extension_head;
					get_value_from_element_from_tuple(&extension_head, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
					extension_head_page_id = extension_head.uint_value;
				}

				// if it is NULL_PAGE_ID, then create a new worm and set it in the attribute beside prefix
				if(extension_head_page_id == dwi_p->pam_p->pas.NULL_PAGE_ID)
				{
					extension_head_page_id = get_new_worm(1, dwi_p->pam_p->pas.NULL_PAGE_ID, dwi_p->wtd_p, dwi_p->pam_p, dwi_p->pmm_p, transaction_id, abort_error);
					if(*abort_error)
					{
						deinitialize_relative_positional_accessor(&child_relative_accessor);
						return 0;
					}
					set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, &((datum){.uint_value = extension_head_page_id}), UINT32_MAX);
				}

				// open a new wai
				dwi_p->wai_p = get_new_worm_append_iterator(extension_head_page_id, dwi_p->wtd_p, dwi_p->pam_p, dwi_p->pmm_p, transaction_id, abort_error);
				if(*abort_error)
				{
					dwi_p->wai_p = NULL;
					deinitialize_relative_positional_accessor(&child_relative_accessor);
					return 0;
				}
			}

			// genertae a buffer consisting of the digits in a SerializableInteger format of 5 bytes each
			uint32_t buffer_size = digits_size * BYTES_PER_NUMERIC_DIGIT;
			void* buffer = malloc(buffer_size);
			if(buffer == NULL)
				exit(-1);
			for(uint32_t i = 0; i < digits_size; i++)
				serialize_uint64(buffer + i * BYTES_PER_NUMERIC_DIGIT, BYTES_PER_NUMERIC_DIGIT, digits[i]);

			// append to worm
			digits_written_this_iteration = append_to_worm(dwi_p->wai_p, buffer, buffer_size, NULL, NULL, transaction_id, abort_error) / BYTES_PER_NUMERIC_DIGIT;
			// it is assumed that this return value of the above function is a multiple of BYTES_PER_NUMERIC_DIGIT
			// and that it consumes as much data (according to current logic all of the data), as the worm can 
			free(buffer); // we can not forget to discard the buffer after the call
			if(*abort_error)
			{
				delete_worm_append_iterator(dwi_p->wai_p, transaction_id, abort_error);
				dwi_p->wai_p = NULL;
				deinitialize_relative_positional_accessor(&child_relative_accessor);
				return 0;
			}
		}

		if(digits_written_this_iteration == 0)
			break;

		digits += digits_written_this_iteration;
		digits_size -= digits_written_this_iteration;
		digits_written += digits_written_this_iteration;
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);
	return digits_written;
}