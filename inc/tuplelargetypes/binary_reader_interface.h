#ifndef BINARY_READER_INTERFACE_H
#define BINARY_READER_INTERFACE_H

#include<tuplestore/tuple.h>
#include<tuplestore/tuple_def.h>

typedef struct binary_reader_interface binary_reader_interface;
struct binary_reader_interface
{
	void* context;

	int (*is_valid)(binary_reader_interface* bri_p);
	// call below functions only if the above one returns 1

	int (*is_null)(binary_reader_interface* bri_p);
	// call below functions only if the above one returns 1

	uint32_t (*read_bytes_as_stream)(binary_reader_interface* bri_p, char* data, uint32_t data_size, int* error);

	const char* (*peek_bytes_as_stream)(binary_reader_interface* bri_p, uint32_t* data_size, int* error);

	// to be called on an error OR EOF OR when you do not need to read any more data
	void (*close_bytes_stream)(binary_reader_interface* bri_p);
};

#define text_reader_interface binary_reader_interface
#define blob_reader_interface binary_reader_interface

/*
	implementation for a text/blob inside a tuple (extended or inline)
*/

#include<tuplelargetypes/binary_read_iterator.h>

typedef struct intuple_binary_reader_interface_context intuple_binary_reader_interface_context;
struct intuple_binary_reader_interface_context
{
	const tuple_def* tpl_d;
	const void* tupl;
	positional_accessor inline_accessor;
	const worm_tuple_defs* wtd_p;
	const page_access_methods* pam_p;
	const void* transaction_id;
	int* abort_error;

	binary_read_iterator* bri_p;
};

int is_valid_for_intuple_binary_reader_interface(binary_reader_interface* bri_p);
int is_null_for_intuple_binary_reader_interface(binary_reader_interface* bri_p);
uint32_t read_bytes_as_stream_for_intuple_binary_reader_interface(binary_reader_interface* bri_p, char* data, uint32_t data_size, int* error);
const char* peek_bytes_as_stream_for_intuple_binary_reader_interface(binary_reader_interface* bri_p, uint32_t* data_size, int* error);
void close_bytes_stream_for_intuple_binary_reader_interface(binary_reader_interface* bri_p);

#define init_intuple_binary_reader_interface(tpl_d_v, tupl_v, inline_accessor_v, wtd_p_v, pam_p_v, transaction_id_v, abort_error_v) \
(binary_reader_interface){ \
	&(intuple_binary_reader_interface_context){tpl_d_v, tupl_v, inline_accessor_v, wtd_p_v, pam_p_v, transaction_id_v, abort_error_v, NULL}, \
	is_valid_for_intuple_binary_reader_interface, \
	is_null_for_intuple_binary_reader_interface, \
	read_bytes_as_stream_for_intuple_binary_reader_interface, \
	peek_bytes_as_stream_for_intuple_binary_reader_interface, \
	close_bytes_stream_for_intuple_binary_reader_interface, \
};


/*
	implementation for a text/blob inside a user_value (extended or inline)
*/

typedef struct user_value_binary_reader_interface_context user_value_binary_reader_interface_context;
struct user_value_binary_reader_interface_context
{
	user_value uval;
	uint32_t bytes_read;
};

int is_valid_for_user_value_binary_reader_interface(binary_reader_interface* bri_p);
int is_null_for_user_value_binary_reader_interface(binary_reader_interface* bri_p);
uint32_t read_bytes_as_stream_for_user_value_binary_reader_interface(binary_reader_interface* bri_p, char* data, uint32_t data_size, int* error);
const char* peek_bytes_as_stream_for_user_value_binary_reader_interface(binary_reader_interface* bri_p, uint32_t* data_size, int* error);
void close_bytes_stream_for_user_value_binary_reader_interface(binary_reader_interface* bri_p);

#define init_user_value_binary_reader_interface(uval_v) \
(binary_reader_interface){ \
	&(user_value_binary_reader_interface_context){uval_v, 0}, \
	is_valid_for_user_value_binary_reader_interface, \
	is_null_for_user_value_binary_reader_interface, \
	read_bytes_as_stream_for_user_value_binary_reader_interface, \
	peek_bytes_as_stream_for_user_value_binary_reader_interface, \
	close_bytes_stream_for_user_value_binary_reader_interface, \
};

#endif