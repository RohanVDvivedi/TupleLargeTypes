#ifndef JSONB_WRITER_INTERFACE_H
#define JSONB_WRITER_INTERFACE_H

typedef struct jsonb_writer_interface jsonb_writer_interface;
struct jsonb_writer_interface
{
	void* context_p;
	void (*write_jsonb_bytes)(void* context_p, const char* bytes, uint32_t bytes_size, int* error);
};

/*
	implementation for writing to a binary_write_iterator
*/

typedef struct jsonb_to_binary_write_iterator_context jsonb_to_binary_write_iterator_context;
struct jsonb_to_binary_write_iterator_context
{
	binary_write_iterator* bwi_p;
	const void* transaction_id;
	int* abort_error;
};

void jsonb_to_binary_write_iterator_write_jsonb_bytes(void* context_p, const char* bytes, uint32_t bytes_size, int* error);

#define init_jsonb_to_binary_write_iterator_interface(bwi_p_v, transaction_id_v, abort_error_v) \
(binary_reader_interface){ \
	&(jsonb_to_binary_write_iterator_context){bwi_p_v, transaction_id_v, abort_error_v}, \
	jsonb_to_binary_write_iterator_write_jsonb_bytes, \
};

#endif