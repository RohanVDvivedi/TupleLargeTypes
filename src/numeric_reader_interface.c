#include<tuplelargetypes/numeric_reader_interface.h>

/*
	implementation for a text/blob inside a tuple (extended or inline)
*/

int is_valid_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p);
int is_null_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p);
void extract_sign_bits_and_exponent_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p, numeric_sign_bits* sign_bits, int16_t* exponent);
uint32_t read_digits_as_stream_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p, uint64_t* digits, uint32_t digits_size, int* error);
void close_digits_stream_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p);