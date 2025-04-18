#ifndef JSONB_READER_INTERFACE_H
#define JSONB_READER_INTERFACE_H

#include<stdint.h>

typedef struct jsonb_reader_interface jsonb_reader_interface;
struct jsonb_reader_interface
{
	void* context;
	uint32_t (*read_jsonb_bytes)(const jsonb_reader_interface* jri_p, char* bytes, uint32_t bytes_size, int* error);
};

/*
	implementation for reading from a binary_read_iterator
*/

#include<tuplelargetypes/binary_read_iterator.h>

typedef struct jsonb_from_binary_read_iterator_context jsonb_from_binary_read_iterator_context;
struct jsonb_from_binary_read_iterator_context
{
	binary_read_iterator* bri_p;
	const void* transaction_id;
	int* abort_error;
};

uint32_t jsonb_from_binary_read_iterator_read_jsonb_bytes(const jsonb_reader_interface* jri_p, char* bytes, uint32_t bytes_size, int* error);

#define init_jsonb_from_binary_read_iterator_interface(bri_p_v, transaction_id_v, abort_error_v) \
(jsonb_reader_interface){ \
	&(jsonb_from_binary_read_iterator_context){bri_p_v, transaction_id_v, abort_error_v}, \
	jsonb_from_binary_read_iterator_read_jsonb_bytes, \
};

#endif