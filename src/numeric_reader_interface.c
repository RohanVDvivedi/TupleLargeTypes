#include<tuplelargetypes/numeric_reader_interface.h>

/*
	implementation for a text/blob inside a tuple (extended or inline)
*/

int is_valid_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p)
{
	intuple_numeric_reader_interface_context* cntxt = nri_p->context;

	user_value uval;
	if(!get_value_from_element_from_tuple(&uval, cntxt->tpl_d, cntxt->inline_accessor, cntxt->tupl))
		return 0;

	if(is_user_value_NULL(&uval))
		return 1;

	// if not numm, then sign_bits and exponent must exist
	numeric_sign_bits sign_bits; int16_t exponent;
	if(!extract_sign_bits_and_exponent_from_numeric(&sign_bits, &exponent, cntxt->tupl, cntxt->tpl_d, cntxt->inline_accessor))
		return 0;

	return 1;
}

int is_null_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p)
{
	intuple_numeric_reader_interface_context* cntxt = nri_p->context;

	user_value uval;
	if(!get_value_from_element_from_tuple(&uval, cntxt->tpl_d, cntxt->inline_accessor, cntxt->tupl))
		return 0;

	return is_user_value_NULL(&uval);
}

void extract_sign_bits_and_exponent_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p, numeric_sign_bits* sign_bits, int16_t* exponent)
{
	intuple_numeric_reader_interface_context* cntxt = nri_p->context;

	extract_sign_bits_and_exponent_from_numeric(sign_bits, exponent, cntxt->tupl, cntxt->tpl_d, cntxt->inline_accessor);
}

uint32_t read_digits_as_stream_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p, uint64_t* digits, uint32_t digits_size, int* error)
{

}

void close_digits_stream_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p)
{

}