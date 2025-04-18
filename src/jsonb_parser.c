#include<tuplelargetypes/jsonb_parser.h>

// below function reads fixed number of bytes from stream
// else error will be set
static inline void jsonb_read_fixed_number_of_bytes(stream* rs, char* data, uint32_t data_size, int* error)
{
	uint32_t bytes_read = 0;
	while(bytes_read < data_size)
	{
		uint32_t bytes_read_this_iteration = read_from_stream(rs, data + bytes_read, data_size - bytes_read, error);
		if(*error)
			return;
		if(bytes_read_this_iteration == 0)
		{
			(*error) = 1;
			return;
		}
		bytes_read += bytes_read_this_iteration;
	}
}

static inline uint8_t jsonb_read_uint8(stream* rs, int* error)
{
	char byte;
	jsonb_read_fixed_number_of_bytes(rs, &byte, 1, error);
	if(*error)
		return 0;
	return (uint8_t)byte;
}

static inline uint32_t jsonb_read_uint32(stream* rs, int* error)
{
	char bytes[4];
	jsonb_read_fixed_number_of_bytes(rs, bytes, 4, error);
	if(*error)
		return 0;
	return deserialize_uint32(bytes, 4);
}

static inline int16_t jsonb_read_int16(stream* rs, int* error)
{
	char bytes[2];
	jsonb_read_fixed_number_of_bytes(rs, bytes, 2, error);
	if(*error)
		return 0;
	return deserialize_int16(bytes, 2);
}

jsonb_node* jsonb_parse(stream* rs)
{
	int error = 0;
	jsonb_node* node_p = NULL;

	uint8_t type = jsonb_read_uint8(rs, &error);
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
			uint32_t skip_size = jsonb_read_uint32(rs, &error);
			if(error)
				return NULL;

			dstring str;
			if(!init_empty_dstring(&str, skip_size))
				exit(-1);

			jsonb_read_fixed_number_of_bytes(rs, get_byte_array_dstring(&str), skip_size, &error);
			if(error)
			{
				deinit_dstring(&str);
				return NULL;
			}
			increment_char_count_dstring(&str, skip_size);

			node_p = get_jsonb_string_node2(str);
			break;
		}
		case JSONB_NUMERIC :
		{
			uint32_t skip_size = jsonb_read_uint32(rs, &error);
			if(error)
				return NULL;

			if((skip_size % 5) != 3) // make sure that there are 5 * N + 3 bytes in the numeric
				return NULL;

			uint8_t sign_bits = jsonb_read_uint8(rs, &error);
			if(error)
				return NULL;

			int16_t exponent = jsonb_read_int16(rs, &error);
			if(error)
				return NULL;

			// TODO
			// intiialize node_p and
			// read digits and put it here
			break;
		}
		case JSONB_ARRAY :
		{
			uint32_t skip_size = jsonb_read_uint32(rs, &error);
			if(error)
				return NULL;

			uint32_t element_count = jsonb_read_uint32(rs, &error);
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
			uint32_t skip_size = jsonb_read_uint32(rs, &error);
			if(error)
				return NULL;

			uint32_t element_count = jsonb_read_uint32(rs, &error);
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