#ifndef BINARY_READER_INTERFACE_H
#define BINARY_READER_INTERFACE_H

typedef struct binary_reader_interface binary_reader_interface;
struct binary_reader_interface
{
	const void* context;

	int (*is_valid)(binary_reader_interface* bri_p);
	// call below functions only if the above one returns 1

	int (*is_null)(binary_reader_interface* bri_p);
	// call below functions only if the above one returns 1

	uint32_t (*read_bytes_as_stream)(binary_read_iterator* bri_p, char* data, uint32_t data_size, int* error);
};

#endif