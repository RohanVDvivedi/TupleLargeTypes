#include<tuplelargetypes/jsonb_parse.h>

static inline uint8_t jsonb_reader_interface_read_uint8(const jsonb_reader_interface* jri_p, int* error)
{
	char byte;
	if(1 != jri_p->read_jsonb_bytes(jri_p, &byte, 1, error))
	{
		(*error) = 1;
		return 0;
	}
	return (uint8_t)byte;
}

static inline uint32_t jsonb_reader_interface_read_uint32(const jsonb_reader_interface* jri_p, int* error)
{
	char bytes[4];
	if(4 != jri_p->read_jsonb_bytes(jri_p, bytes, 4, error))
	{
		(*error) = 1;
		return 0;
	}
	return deserialize_uint32(bytes, 4);
}

static inline int16_t jsonb_reader_interface_read_int16(const jsonb_reader_interface* jri_p, int* error)
{
	char bytes[2];
	if(2 != jri_p->read_jsonb_bytes(jri_p, bytes, 2, error))
	{
		(*error) = 1;
		return 0;
	}
	return deserialize_int16(bytes, 2);
}

jsonb_node* jsonb_parse(jsonb_reader_interface* jri_p)
{
	// TODO
}