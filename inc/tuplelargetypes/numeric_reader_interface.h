#ifndef NUMERIC_READER_INTERFACE_H
#define NUMERIC_READER_INTERFACE_H

#include<tuplestore/tuple.h>
#include<tuplestore/tuple_def.h>

typedef struct numeric_reader_interface numeric_reader_interface;

#include<tuplelargetypes/numeric_extended.h>

struct numeric_reader_interface
{
	void* context;

	int (*is_valid)(numeric_reader_interface* nri_p);
	// call below functions only if the above one returns 1

	int (*is_null)(numeric_reader_interface* nri_p);
	// call below functions only if the above one returns 1

	void (*extract_sign_bits_and_exponent)(numeric_reader_interface* nri_p, numeric_sign_bits* sign_bits, int16_t* exponent);

	uint32_t (*read_digits_as_stream)(numeric_reader_interface* nri_p, uint64_t* digits, uint32_t digits_size, int* error);

	// to be called on an error OR EOF OR when you do not need to read any more data
	void (*close_digits_stream)(numeric_reader_interface* nri_p);
};

/*
	implementation for a numeric inside a tuple (extended or inline)
*/

#include<tuplelargetypes/digit_read_iterator.h>

typedef struct intuple_numeric_reader_interface_context intuple_numeric_reader_interface_context;
struct intuple_numeric_reader_interface_context
{
	const tuple_def* tpl_d;
	const void* tupl;
	positional_accessor inline_accessor;
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
	const void* transaction_id;
	int* abort_error;

	digit_read_iterator* dri_p;
};

int is_valid_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p);
int is_null_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p);
void extract_sign_bits_and_exponent_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p, numeric_sign_bits* sign_bits, int16_t* exponent);
uint32_t read_digits_as_stream_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p, uint64_t* digits, uint32_t digits_size, int* error);
void close_digits_stream_for_intuple_numeric_reader_interface(numeric_reader_interface* nri_p);

#define init_intuple_numeric_reader_interface(tpl_d_v, tupl_v, inline_accessor_v, wtd_p_v, pam_p_v, transaction_id_v, abort_error_v) \
(numeric_reader_interface){ \
	&(intuple_numeric_reader_interface_context){tpl_d_v, tupl_v, inline_accessor_v, wtd_p_v, pam_p_v, transaction_id_v, abort_error_v, NULL}, \
	is_valid_for_intuple_numeric_reader_interface, \
	is_null_for_intuple_numeric_reader_interface, \
	extract_sign_bits_and_exponent_for_intuple_numeric_reader_interface, \
	read_digits_as_stream_for_intuple_numeric_reader_interface, \
	close_digits_stream_for_intuple_numeric_reader_interface, \
};

/*
	implementation for a materialized_numeric
*/

typedef struct materialized_numeric_reader_interface_context materialized_numeric_reader_interface_context;
struct materialized_numeric_reader_interface_context
{
	const materialized_numeric* m;

	uint32_t digits_processed;
};

int is_valid_for_materialized_numeric_reader_interface(numeric_reader_interface* nri_p);
int is_null_for_materialized_numeric_reader_interface(numeric_reader_interface* nri_p);
void extract_sign_bits_and_exponent_for_materialized_numeric_reader_interface(numeric_reader_interface* nri_p, numeric_sign_bits* sign_bits, int16_t* exponent);
uint32_t read_digits_as_stream_for_materialized_numeric_reader_interface(numeric_reader_interface* nri_p, uint64_t* digits, uint32_t digits_size, int* error);
void close_digits_stream_for_materialized_numeric_reader_interface(numeric_reader_interface* nri_p);

#define init_materialized_numeric_reader_interface(m_v) \
(numeric_reader_interface){ \
	&(materialized_numeric_reader_interface_context){m_v, 0}, \
	is_valid_for_materialized_numeric_reader_interface, \
	is_null_for_materialized_numeric_reader_interface, \
	extract_sign_bits_and_exponent_for_materialized_numeric_reader_interface, \
	read_digits_as_stream_for_materialized_numeric_reader_interface, \
	close_digits_stream_for_materialized_numeric_reader_interface, \
};

#endif