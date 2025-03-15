#include<digit_read_iterator.h>

#include<page_access_methods.h>

#include<stdlib.h>

#include<relative_positional_accessor.h>

#include<digit_iterator_commons.h>

digit_read_iterator* get_new_digit_read_iterator(const void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p)
{
	digit_read_iterator* dri_p = malloc(sizeof(digit_read_iterator));
	if(dri_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	dri_p->is_inline = is_inline_type_info(dti_p);

	dri_p->tupl = tupl;
	dri_p->tpl_d = tpl_d;
	dri_p->inline_accessor = inline_accessor;

	dri_p->digits_read_from_prefix = 0;

	dri_p->wri_p = NULL;

	dri_p->wtd_p = wtd_p;
	dri_p->pam_p = pam_p;

	return dri_p;
}

void delete_digit_read_iterator(digit_read_iterator* dri_p, const void* transaction_id, int* abort_error)
{
	if(dri_p->wri_p != NULL)
		delete_worm_read_iterator(dri_p->wri_p, transaction_id, abort_error);
	free(dri_p);
}

digit_read_iterator* clone_digit_read_iterator(digit_read_iterator* dri_p, const void* transaction_id, int* abort_error)
{
	digit_read_iterator* clone_p = malloc(sizeof(digit_read_iterator));
	if(clone_p == NULL)
		exit(-1);

	(*clone_p) = (*dri_p);

	if((!clone_p->is_inline) && (dri_p->wri_p != NULL)) // if is_large && wri_p != NULL then a wri_p clone is necessary
	{
		clone_p->wri_p = clone_worm_read_iterator(dri_p->wri_p, transaction_id, abort_error);
		if(*abort_error)
		{
			free(clone_p);
			return NULL;
		}
	}

	return clone_p;
}

uint32_t read_from_digit_read_iterator(digit_read_iterator* dri_p, uint64_t* digits, uint32_t digits_size, const void* transaction_id, int* abort_error)
{
	if(digits_size == 0)
		return 0;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(dri_p->inline_accessor), 3);

	// cache the prefix_digits_count and the extension_head_page_id in local variables
	uint32_t prefix_digits_count = 0;
	uint64_t extension_head_page_id = dri_p->pam_p->pas.NULL_PAGE_ID;
	{
		if(dri_p->is_inline)
		{
			point_to_prefix(&child_relative_accessor, dri_p->is_inline);
			user_value prefix;
			int valid_prefix = get_value_from_element_from_tuple(&prefix, dri_p->tpl_d, child_relative_accessor.exact, dri_p->tupl);
			if(valid_prefix && !is_user_value_NULL(&prefix))
				prefix_digits_count = get_element_count_for_element_from_tuple(dri_p->tpl_d, child_relative_accessor.exact, dri_p->tupl);
		}
		else
		{
			point_to_prefix(&child_relative_accessor, dri_p->is_inline);
			user_value prefix;
			int valid_prefix = get_value_from_element_from_tuple(&prefix, dri_p->tpl_d, child_relative_accessor.exact, dri_p->tupl);
			if(valid_prefix && !is_user_value_NULL(&prefix))
			{
				prefix_digits_count = get_element_count_for_element_from_tuple(dri_p->tpl_d, child_relative_accessor.exact, dri_p->tupl);

				// this means a valid extension_head_page_id for the worm may exist
				if(dri_p->wri_p == NULL) // you will need extension_head_page_id set to valid value only if the worm_read_iterator is not initialized
				{
					user_value worm_head_page_id;
					point_to_extension_head_page_id(&child_relative_accessor, dri_p->is_inline);
					int valid_worm_head_page_id = get_value_from_element_from_tuple(&worm_head_page_id, dri_p->tpl_d, child_relative_accessor.exact, dri_p->tupl);
					if(valid_worm_head_page_id && !is_user_value_NULL(&worm_head_page_id)) // if valid and not NULL
						extension_head_page_id = worm_head_page_id.uint_value;
				}
			}
			else
			{
				prefix_digits_count = 0;
				extension_head_page_id = dri_p->pam_p->pas.NULL_PAGE_ID;
			}
		}
	}

	uint32_t digits_read = 0;

	while(digits_size > 0)
	{
		uint32_t digits_read_this_iteration = 0;

		if(dri_p->digits_read_from_prefix < prefix_digits_count) // read from prefix until it is not completely read
		{
			digits_read_this_iteration = min(digits_size, prefix_digits_count - dri_p->digits_read_from_prefix);
			if(digits != NULL)
			{
				for(uint32_t i = 0; i < digits_read_this_iteration; i++)
				{
					point_to_prefix_s_digit(&child_relative_accessor, dri_p->digits_read_from_prefix + i, dri_p->is_inline);
					user_value digit_value;
					get_value_from_element_from_tuple(&digit_value, dri_p->tpl_d, child_relative_accessor.exact, dri_p->tupl);
					digits[i] = digit_value.uint_value;
				}
			}
			dri_p->digits_read_from_prefix += digits_read_this_iteration;
		}
		else if(!(dri_p->is_inline)) // go here only if it is a large text or blob
		{
			// initialize worm read iterator if not done already
			if(dri_p->wri_p == NULL)
			{
				if(extension_head_page_id == dri_p->pam_p->pas.NULL_PAGE_ID)
					goto NOTHING_TO_READ;

				dri_p->wri_p = get_new_worm_read_iterator(extension_head_page_id, dri_p->wtd_p, dri_p->pam_p, transaction_id, abort_error);
				if(*abort_error) // on abort error, do nothing
				{
					dri_p->wri_p = NULL;
					deinitialize_relative_positional_accessor(&child_relative_accessor);
					return 0;
				}
			}

			// we will read data from worm directly into the digits buffer but at the end of this buffer
			const uint32_t buffer_size = (digits_size * 5); // 5 bytes per digit will require this many bytes
			void * const buffer = ((void*)(digits + digits_size)) - buffer_size; // (end of digits) - buffer_size

			// perform actual read
			const uint32_t bytes_read_this_iteration = read_from_worm(dri_p->wri_p, buffer, buffer_size, transaction_id, abort_error); // current code assumes that bytes_read_this_iteration will always be multiple of 5
			if(*abort_error) // on abort error, delete worm iterator and set it to NULL
			{
				delete_worm_read_iterator(dri_p->wri_p, transaction_id, abort_error);
				dri_p->wri_p = NULL;
				deinitialize_relative_positional_accessor(&child_relative_accessor);
				return 0;
			}

			// now convert bytes into digits
			digits_read_this_iteration = bytes_read_this_iteration / 5; // current code assumes that bytes_read_this_iteration will always be multiple of 5
			for(uint32_t i = 0; i < digits_read_this_iteration; i++)
				digits[i] = deserialize_uint64(buffer + (i * 5), 5);
		}

		// skip label to goto, if nothing to be read is found
		NOTHING_TO_READ:;

		if(digits_read_this_iteration == 0)
			break;

		digits += digits_read_this_iteration;
		digits_size -= digits_read_this_iteration;
		digits_read += digits_read_this_iteration;
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);

	return digits_read;
}