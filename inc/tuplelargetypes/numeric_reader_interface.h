#ifndef NUMERIC_READER_INTERFACE_H
#define NUMERIC_READER_INTERFACE_H

typedef struct numeric_reader_interface numeric_reader_interface;
struct numeric_reader_interface
{
	const void* context;

	int (*is_valid)(numeric_reader_interface* nri_p);
	// call below functions only if the above one returns 1

	int (*is_null)(numeric_reader_interface* nri_p);
	// call below functions only if the above one returns 1

	void (*extract_sign_bits_and_exponent)(numeric_reader_interface* nri_p, numeric_sign_bits* sign_bits, int16_t* exponent);

	uint32_t (*read_digits_as_stream)(numeric_reader_interface* nri_p, uint64_t* digits, uint32_t digits_size, int* error);

	// to be called on an error OR EOF OR when you do not need to read any more data
	void (*close_digits_stream)(numeric_reader_interface* nri_p);
};

#endif