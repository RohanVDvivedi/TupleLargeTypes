#include<tuplelargetypes/digit_read_iterator.h>

#include<tupleindexer/interface/page_access_methods.h>

#include<stdlib.h>

#include<tuplelargetypes/relative_positional_accessor.h>

#include<tuplelargetypes/numeric_extended.h>

digit_read_iterator* get_new_digit_read_iterator(const datum* uval, const data_type_info* dti, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p)
{
	digit_read_iterator* dri_p = malloc(sizeof(digit_read_iterator));
	if(dri_p == NULL)
		exit(-1);

	dri_p->digits_inline = (*NULL_DATUM);
	dri_p->digits_dti = NULL;

	dri_p->digits_inline_count = 0;
	dri_p->digits_inline_read = 0;

	dri_p->extension_head_page_id = pam_p->pas.NULL_PAGE_ID;
	dri_p->wri_p = NULL;

	dri_p->wtd_p = wtd_p;
	dri_p->pam_p = pam_p;

	if(is_extended_type_info(dti))
		dri_p->extension_head_page_id = get_extension_head_page_id_for_extended_type(uval, dti, &(pam_p->pas));

	{
		datum prefix_digits;
		const data_type_info* prefix_digits_dti;
		int valid = get_nested_containee_from_datum(&prefix_digits, &prefix_digits_dti, uval, dti, GET_NUMERIC_DIGITS_POS_ACC(is_extended_type_info(dti)));
		if(valid && !is_datum_NULL(&prefix_digits))
		{
			dri_p->digits_inline = prefix_digits;
			dri_p->digits_dti = prefix_digits_dti;
			dri_p->digits_inline_count = get_element_count_for_datum(&prefix_digits, prefix_digits_dti);
		}
	}

	return dri_p;
}

void delete_digit_read_iterator(digit_read_iterator* dri_p, const void* transaction_id, int* abort_error)
{
	if(dri_p->wri_p != NULL)
		delete_worm_read_iterator(dri_p->wri_p, transaction_id, abort_error);
	free(dri_p);
}

digit_read_iterator* clone_digit_read_iterator(const digit_read_iterator* dri_p, const void* transaction_id, int* abort_error)
{
	digit_read_iterator* clone_p = malloc(sizeof(digit_read_iterator));
	if(clone_p == NULL)
		exit(-1);

	(*clone_p) = (*dri_p);

	if(dri_p->wri_p != NULL)
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
	if(is_datum_NULL(&(dri_p->digits_inline)))
		return 0;

	uint32_t digits_read = 0;

	while(digits_read < digits_size)
	{
		uint32_t digits_read_this_iteration = 0;

		if(dri_p->digits_inline_read < dri_p->digits_inline_count) // bytes in prefix yet to be read
		{
			datum digit_val;
			const data_type_info* digit_dti;
			get_containee_from_datum(&digit_val, &digit_dti, &(dri_p->digits_inline), dri_p->digits_dti, dri_p->digits_inline_read++);
			if(digits != NULL)
				digits[0] = digit_val.uint_value;

			digits_read_this_iteration++;
		}
		else if(dri_p->extension_head_page_id != dri_p->pam_p->pas.NULL_PAGE_ID)
		{
			// initialize worm read iterator if not done already
			if(dri_p->wri_p == NULL)
			{
				dri_p->wri_p = get_new_worm_read_iterator(dri_p->extension_head_page_id, dri_p->wtd_p, dri_p->pam_p, transaction_id, abort_error);
				if(*abort_error)
					return 0;
			}

			// we will read data from worm directly into the digits buffer but at the end of this buffer
			const uint32_t buffer_size = (digits_size * BYTES_PER_NUMERIC_DIGIT); // BYTES_PER_NUMERIC_DIGIT bytes per digit will require this many bytes
			void * const buffer = (digits == NULL) ? NULL : (((void*)(digits + digits_size)) - buffer_size); // (end of digits) - buffer_size

			// perform actual read
			const uint32_t bytes_read_this_iteration = read_from_worm(dri_p->wri_p, buffer, buffer_size, transaction_id, abort_error); // current code assumes that bytes_read_this_iteration will always be multiple of BYTES_PER_NUMERIC_DIGIT
			if(*abort_error) // on abort error, delete worm iterator and set it to NULL
				return 0;

			// now convert bytes into digits
			// this copy without fail will ensure that a write happens after the read and at a location such that subsequent reads are not affected, it may overwrite the currently read bytes, DEEP BUT TRUE, this is the only way you can do it without additional buffer allocation
			digits_read_this_iteration = bytes_read_this_iteration / BYTES_PER_NUMERIC_DIGIT; // current code assumes that bytes_read_this_iteration will always be multiple of BYTES_PER_NUMERIC_DIGIT
			if(digits)
				for(uint32_t i = 0; i < digits_read_this_iteration; i++)
					digits[i] = deserialize_uint64(buffer + (i * BYTES_PER_NUMERIC_DIGIT), BYTES_PER_NUMERIC_DIGIT);
		}

		if(digits_read_this_iteration == 0)
			break;

		if(digits)
			digits += digits_read_this_iteration;
		digits_size -= digits_read_this_iteration;
		digits_read += digits_read_this_iteration;
	}

	return digits_read;
}