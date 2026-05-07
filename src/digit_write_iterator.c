#include<tuplelargetypes/digit_write_iterator.h>

#include<tupleindexer/interface/page_access_methods.h>

#include<stdlib.h>

#include<tuplelargetypes/relative_positional_accessor.h>

#include<tuplelargetypes/numeric_extended.h>

digit_write_iterator* get_new_digit_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor pos, uint64_t blob_store_root_page_id, tuple_pointer extension_tail, uint32_t digits_to_be_written_to_prefix, const blob_store_tuple_defs* bstd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p)
{
	digit_write_iterator* dwi_p = malloc(sizeof(digit_write_iterator));
	if(dwi_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, pos);
	dwi_p->is_extended = is_extended_type_info(dti_p);

	dwi_p->tupl = tupl;
	dwi_p->tpl_d = tpl_d;
	dwi_p->pos = pos;

	dwi_p->blob_store_root_page_id = blob_store_root_page_id;

	dwi_p->extension_head = get_NULL_tuple_pointer(&(pam_p->pas));
	dwi_p->extension_tail = get_NULL_tuple_pointer(&(pam_p->pas));

	dwi_p->bswi_p = NULL;

	dwi_p->bstd_p = bstd_p;
	dwi_p->pam_p = pam_p;
	dwi_p->pmm_p = pmm_p;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(dwi_p->pos), 3);

	if(dwi_p->is_extended)
	{
		relative_positonal_accessor_set_from_relative(&child_relative_accessor, GET_NUMERIC_DIGITS_POS_ACC(dwi_p->is_extended));
		datum prefix;
		get_value_from_element_from_tuple(&prefix, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);

		if(is_datum_NULL(&prefix))
		{
			set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, EMPTY_DATUM, UINT32_MAX);

			set_extension_head_for_extended_type(tupl, tpl_d, pos, &(pam_p->pas), get_NULL_tuple_pointer(&(pam_p->pas)));

			dwi_p->digits_written_to_prefix = 0;
			dwi_p->digits_to_be_written_to_prefix = digits_to_be_written_to_prefix;
		}
		else // note down the number of bytes already written to the prefix
		{
			dwi_p->digits_written_to_prefix = get_element_count_for_element_from_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
			dwi_p->digits_to_be_written_to_prefix = digits_to_be_written_to_prefix;

			// initialize extension pointers, only needed here
			{
				datum uval;
				get_value_from_element_from_tuple(&uval, tpl_d, pos, tupl);
				dwi_p->extension_head = get_extension_head_for_extended_type(&uval, dti_p, &(pam_p->pas));
				dwi_p->extension_tail = extension_tail;
			}

			// if the extension_head_page_id is not NULL, then we already wrote the prefix completely
			if(is_tuple_pointer_NULL(dwi_p->extension_head, &(dwi_p->pam_p->pas)))
				dwi_p->digits_to_be_written_to_prefix = dwi_p->digits_written_to_prefix;
		}

		// limit the bytes_to_be_written_to_prefix, by the amount of bytes the tuple can allow us to expand it
		dwi_p->digits_to_be_written_to_prefix = dwi_p->digits_written_to_prefix +
			min(dwi_p->digits_to_be_written_to_prefix - dwi_p->digits_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl) / BYTES_PER_NUMERIC_DIGIT);
	}
	else
	{
		relative_positonal_accessor_set_from_relative(&child_relative_accessor, GET_NUMERIC_DIGITS_POS_ACC(dwi_p->is_extended));
		datum prefix;
		get_value_from_element_from_tuple(&prefix, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);

		if(is_datum_NULL(&prefix))
		{
			set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, EMPTY_DATUM, UINT32_MAX);
			get_value_from_element_from_tuple(&prefix, dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
		}

		dwi_p->digits_written_to_prefix = get_element_count_for_element_from_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
		dwi_p->digits_to_be_written_to_prefix = digits_to_be_written_to_prefix;

		// limit the bytes_to_be_written_to_prefix, by the amount of bytes the tuple can allow us to expand it
		dwi_p->digits_to_be_written_to_prefix = dwi_p->digits_written_to_prefix +
			min(dwi_p->digits_to_be_written_to_prefix - dwi_p->digits_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl) / BYTES_PER_NUMERIC_DIGIT);
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);
	return dwi_p;
}

void delete_digit_write_iterator(digit_write_iterator* dwi_p, const void* transaction_id, int* abort_error)
{
	if(dwi_p->bswi_p != NULL)
		delete_blob_store_write_iterator(dwi_p->bswi_p, transaction_id, abort_error);
	free(dwi_p);
}

uint32_t append_to_digit_write_iterator(digit_write_iterator* dwi_p, const uint64_t* digits, uint32_t digits_size, const heap_table_notifier* notify_wrong_entry, const void* transaction_id, int* abort_error)
{
	int need_to_update_extension_head = 0;
	int need_to_update_extension_tail = 0;

	if(digits_size == 0)
		return 0;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(dwi_p->pos), 3);

	uint32_t digits_written = 0;

	while(digits_size > 0)
	{
		uint32_t digits_written_this_iteration = 0;

		if(dwi_p->digits_written_to_prefix < dwi_p->digits_to_be_written_to_prefix)
		{
			digits_written_this_iteration = min(digits_size, dwi_p->digits_to_be_written_to_prefix - dwi_p->digits_written_to_prefix);

			// grab old_element_count and expand the container
			relative_positonal_accessor_set_from_relative(&child_relative_accessor, GET_NUMERIC_DIGITS_POS_ACC(dwi_p->is_extended));
			uint32_t old_element_count = get_element_count_for_element_from_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl);
			expand_element_count_for_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, old_element_count, digits_written_this_iteration, digits_written_this_iteration * BYTES_PER_NUMERIC_DIGIT);

			// copy data into it digit by digit
			point_to_i_th_child_position(&(child_relative_accessor.exact), old_element_count);
			for(uint32_t i = 0; i < digits_written_this_iteration; i++, point_to_next_sibling_position(&(child_relative_accessor.exact)))
				set_element_in_tuple(dwi_p->tpl_d, child_relative_accessor.exact, dwi_p->tupl, &((datum){.uint_value = digits[i]}), UINT32_MAX);

			dwi_p->digits_written_to_prefix += digits_written_this_iteration;
		}
		else if(dwi_p->is_extended)
		{
			if(dwi_p->bswi_p == NULL)
			{
				// open a new bswi
				dwi_p->bswi_p = get_new_blob_store_write_iterator(dwi_p->blob_store_root_page_id, dwi_p->extension_head, dwi_p->extension_tail, dwi_p->bstd_p, dwi_p->pam_p, dwi_p->pmm_p, transaction_id, abort_error);
				if(*abort_error)
				{
					deinitialize_relative_positional_accessor(&child_relative_accessor);
					return 0;
				}
			}

			// append digits to worm

			// generate a buffer consisting of the digits in a SerializableInteger format of 5 bytes each
			uint32_t buffer_size = digits_size * BYTES_PER_NUMERIC_DIGIT;
			void* buffer = malloc(buffer_size);
			if(buffer == NULL)
				exit(-1);
			for(uint32_t i = 0; i < digits_size; i++)
				serialize_uint64(buffer + i * BYTES_PER_NUMERIC_DIGIT, BYTES_PER_NUMERIC_DIGIT, digits[i]);

			// append to blob in blob_store
			digits_written_this_iteration = digits_size;
			uint32_t bytes_written = 0;
			while(bytes_written < buffer_size)
			{
				uint32_t bytes_written_this_iteration = append_to_tail_in_blob(dwi_p->bswi_p, buffer + bytes_written, buffer_size - bytes_written, NULL, notify_wrong_entry, transaction_id, abort_error) / BYTES_PER_NUMERIC_DIGIT;
				if(*abort_error)
					break;
				if(bytes_written_this_iteration == 0)
					break;
				bytes_written += bytes_written_this_iteration;
			}
			free(buffer);
			if(*abort_error)
			{
				deinitialize_relative_positional_accessor(&child_relative_accessor);
				return 0;
			}

			// whether to update chunk_ptrs to head and tail
			// only update head if it was previously NULL_PAGE_ID
			need_to_update_extension_head = need_to_update_extension_head || ((digits_written_this_iteration > 0) && is_tuple_pointer_NULL(dwi_p->extension_head, &(dwi_p->pam_p->pas)));
			need_to_update_extension_tail = need_to_update_extension_tail || (digits_written_this_iteration > 0);
		}

		if(digits_written_this_iteration == 0)
			break;

		digits += digits_written_this_iteration;
		digits_size -= digits_written_this_iteration;
		digits_written += digits_written_this_iteration;
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);

	if(need_to_update_extension_head)
	{
		dwi_p->extension_head = get_head_pointer_in_blob(dwi_p->bswi_p);
		set_extension_head_for_extended_type(dwi_p->tupl, dwi_p->tpl_d, dwi_p->pos, &(dwi_p->pam_p->pas), dwi_p->extension_head);
	}

	if(need_to_update_extension_tail)
		dwi_p->extension_tail = get_tail_pointer_in_blob(dwi_p->bswi_p);

	return digits_written;
}