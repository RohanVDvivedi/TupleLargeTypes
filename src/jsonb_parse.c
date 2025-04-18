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
	int error = 0;
	jsonb_node* node_p = NULL;

	uint8_t type = jsonb_reader_interface_read_uint8(jri_p, &error);
	if(error)
		return NULL;

	switch(type)
	{
		case JSONB_NULL :
			return &jsonb_null;
		case JSONB_TRUE :
			return &jsonb_true;
		case JSONB_FALSE :
			return &jsonb_false;
		case JSONB_STRING :
		{
			uint32_t skip_size = jsonb_reader_interface_read_uint32(jri_p, &error);
			if(error)
				return NULL;

			// TODO
			// intiialize node_p and
			// parse string and put it here
			break;
		}
		case JSONB_NUMERIC :
		{
			uint32_t skip_size = jsonb_reader_interface_read_uint32(jri_p, &error);
			if(error)
				return NULL;

			if((skip_size % 5) != 3) // make sure that there are 5 * N + 3 bytes in the numeric
				return NULL;

			uint8_t sign_bits = jsonb_reader_interface_read_uint8(jri_p, &error);
			if(error)
				return NULL;

			int16_t exponent = jsonb_reader_interface_read_int16(jri_p, &error);
			if(error)
				return NULL;

			// TODO
			// intiialize node_p and
			// read digits and put it here
			break;
		}
		case JSONB_ARRAY :
		{
			uint32_t skip_size = jsonb_reader_interface_read_uint32(jri_p, &error);
			if(error)
				return NULL;

			uint32_t element_count = jsonb_reader_interface_read_uint32(jri_p, &error);
			if(error)
				return NULL;

			// TODO
			// intiialize node_p

			// read element_count jsonb_node-s and insert them
			for(uint32_t i = 0; i < element_count; i++)
			{
				// TODO
			}

			break;
		}
		case JSONB_OBJECT :
		{
			uint32_t skip_size = jsonb_reader_interface_read_uint32(jri_p, &error);
			if(error)
				return NULL;

			uint32_t element_count = jsonb_reader_interface_read_uint32(jri_p, &error);
			if(error)
				return NULL;

			// TODO
			// intiialize node_p

			// read element_count jsonb_object_entry-s and insert them
			for(uint32_t i = 0; i < element_count; i++)
			{
				// TODO
				// parse string
				// parse value as jsonb_node
			}

			break;
		}
	}

	return node_p;
}