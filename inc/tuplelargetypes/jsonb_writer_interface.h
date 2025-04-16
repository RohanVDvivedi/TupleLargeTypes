#ifndef JSONB_WRITER_INTERFACE_H
#define JSONB_WRITER_INTERFACE_H

typedef struct jsonb_writer_interface jsonb_writer_interface;
struct jsonb_writer_interface
{
	void* context_p;
	void (*write_jsonb_bytes)(void* context_p, const char* bytes, uint32_t bytes_size, int* error);
};

#endif