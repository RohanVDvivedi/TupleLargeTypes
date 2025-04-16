#include<tuplelargetypes/jsonb_serialize.h>

#include<serint/serial_int.h>

static inline void jsonb_writer_interface_write_uint8(jsonb_writer_interface* jwi_p, uint8_t b, int* error)
{
	char byte = b;
	jwi_p->write_jsonb_bytes(jwi_p, &byte, 1, error);
}

static inline void jsonb_writer_interface_write_uint32(jsonb_writer_interface* jwi_p, uint32_t b, int* error)
{
	char bytes[4];
	serialize_uint32(bytes, 4, b);
	jwi_p->write_jsonb_bytes(jwi_p, bytes, 4, error);
}

int jsonb_serialize(const jsonb_writer_interface* jwi_p, jsonb_node* node_p)
{
	// TODO
}