#include<tuplelargetypes/numeric_reader_interface.h>

/*
	implementation for a text/binary inside a tuple (extended or inline)
*/

int is_null_for_intuple_numeric_reader_interface(const numeric_reader_interface* nri_p)
{
	intuple_numeric_reader_interface_context* cntxt = nri_p->context;

	return is_datum_NULL(cntxt->uval);
}

void extract_sign_bits_and_exponent_for_intuple_numeric_reader_interface(const numeric_reader_interface* nri_p, numeric_sign_bits* sign_bits, int16_t* exponent)
{
	intuple_numeric_reader_interface_context* cntxt = nri_p->context;

	extract_sign_bits_and_exponent_from_numeric(sign_bits, exponent, cntxt->uval, cntxt->dti);
}

uint32_t read_digits_as_stream_for_intuple_numeric_reader_interface(const numeric_reader_interface* nri_p, uint64_t* digits, uint32_t digits_size, int* error)
{
	intuple_numeric_reader_interface_context* cntxt = nri_p->context;

	if((*(cntxt->abort_error)))
	{
		(*error) = (*cntxt->abort_error);
		return 0;
	}

	if(cntxt->dri_p == NULL)
		cntxt->dri_p = get_new_digit_read_iterator(cntxt->uval, cntxt->dti, cntxt->wtd_p, cntxt->pam_p);

	digits_size = read_from_digit_read_iterator(cntxt->dri_p, digits, digits_size, cntxt->transaction_id, cntxt->abort_error);
	(*error) = (*cntxt->abort_error);
	if((*(cntxt->abort_error)))
		return 0;

	return digits_size;
}

void close_digits_stream_for_intuple_numeric_reader_interface(const numeric_reader_interface* nri_p)
{
	intuple_numeric_reader_interface_context* cntxt = nri_p->context;
	if(cntxt->dri_p != NULL)
	{
		delete_digit_read_iterator(cntxt->dri_p, cntxt->transaction_id, cntxt->abort_error);
		cntxt->dri_p = NULL;
	}
}

/*
	implementation for a materialized_numeric
*/

int is_null_for_materialized_numeric_reader_interface(const numeric_reader_interface* nri_p)
{
	materialized_numeric_reader_interface_context* cntxt = nri_p->context;
	return (cntxt->m == NULL);
}

void extract_sign_bits_and_exponent_for_materialized_numeric_reader_interface(const numeric_reader_interface* nri_p, numeric_sign_bits* sign_bits, int16_t* exponent)
{
	materialized_numeric_reader_interface_context* cntxt = nri_p->context;
	get_sign_bits_and_exponent_for_materialized_numeric(cntxt->m, sign_bits, exponent);
}

uint32_t read_digits_as_stream_for_materialized_numeric_reader_interface(const numeric_reader_interface* nri_p, uint64_t* digits, uint32_t digits_size, int* error)
{
	materialized_numeric_reader_interface_context* cntxt = nri_p->context;

	uint32_t digits_count = get_digits_count_for_materialized_numeric(cntxt->m);

	uint32_t digits_to_read = min(digits_size, digits_count - cntxt->digits_read);

	if(digits != NULL)
	{
		for(uint32_t i = 0; i < digits_to_read; i++)
			digits[i] = get_nth_digit_from_materialized_numeric(cntxt->m, cntxt->digits_read + i);
	}

	cntxt->digits_read += digits_to_read;

	return digits_to_read;
}

void close_digits_stream_for_materialized_numeric_reader_interface(const numeric_reader_interface* nri_p)
{
}