#include<tuplelargetypes/jsonb_parser.h>

#include<stdlib.h>

#include<cutlery/stream_util.h>

// below function reads fixed sized dstring from stream
static inline dstring jsonb_read_fixed_sized_dstring(stream* rs, uint32_t bytes_to_read, int* error)
{
	dstring res = read_dstring_from_stream(rs, bytes_to_read, error);
	if(*error)
		return res;

	// make sure we read as many bytes as expected
	if(get_char_count_dstring(&res) != bytes_to_read)
	{
		(*error) = 1; // force a fake error in stream, because an EOF encountered before our expected bytes could be read
		deinit_dstring(&res);
		return res;
	}

	return res;
}

static inline uint8_t jsonb_read_uint8(stream* rs, int* error)
{
	dstring res = jsonb_read_fixed_sized_dstring(rs, 1, error);
	if(*error)
		return 0;
	uint8_t uint8_val = (uint8_t)(get_byte_array_dstring(&res)[0]);
	deinit_dstring(&res);
	return uint8_val;
}

static inline uint32_t jsonb_read_uint32(stream* rs, int* error)
{
	dstring res = jsonb_read_fixed_sized_dstring(rs, 4, error);
	if(*error)
		return 0;
	uint32_t uint32_val = deserialize_uint32(get_byte_array_dstring(&res), 4);
	deinit_dstring(&res);
	return uint32_val;
}

static inline int16_t jsonb_read_int16(stream* rs, int* error)
{
	dstring res = jsonb_read_fixed_sized_dstring(rs, 2, error);
	if(*error)
		return 0;
	int16_t int16_val = deserialize_int16(get_byte_array_dstring(&res), 2);
	deinit_dstring(&res);
	return int16_val;
}

jsonb_node* parse_jsonb(stream* rs)
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

			dstring str = jsonb_read_fixed_sized_dstring(rs, skip_size, &error);
			if(error)
				return NULL;

			node_p = new_jsonb_string_node2(str); // str is consumed hence no need to deinitialize it

			node_p->skip_size = skip_size;
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

			materialized_numeric m;
			if(!initialize_materialized_numeric(&m, skip_size/5))
				exit(-1);
			set_sign_bits_and_exponent_for_materialized_numeric(&m, sign_bits, exponent);

			// read digits and put it here
			for(uint32_t i = 0; i < skip_size/5; i++)
			{
				dstring s_digit = jsonb_read_fixed_sized_dstring(rs, 5, &error);
				if(error)
				{
					deinitialize_materialized_numeric(&m);
					return NULL;
				}
				uint64_t digit = deserialize_uint64(get_byte_array_dstring(&s_digit), 5);
				deinit_dstring(&s_digit);

				push_lsd_in_materialized_numeric(&m, digit);
			}

			node_p = new_jsonb_numeric_node2(m);

			node_p->skip_size = skip_size;
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

			// intiialize node_p
			node_p = new_jsonb_array_node(element_count);

			// read element_count jsonb_node-s and insert them
			for(uint32_t i = 0; i < element_count; i++)
			{
				jsonb_node* n_p = parse_jsonb(rs);
				if(n_p == NULL)
				{
					delete_jsonb_node(node_p);
					return NULL;
				}

				if(!push_in_jsonb_array_node(node_p, n_p))
				{
					delete_jsonb_node(node_p);
					delete_jsonb_node(n_p);
					return NULL;
				}
			}

			node_p->skip_size = skip_size;
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

			// intiialize node_p
			node_p = new_jsonb_object_node();

			// read element_count jsonb_object_entry-s and insert them
			for(uint32_t i = 0; i < element_count; i++)
			{
				// parse string
				uint32_t key_size = jsonb_read_uint32(rs, &error);
				if(error)
				{
					delete_jsonb_node(node_p);
					return NULL;
				}

				dstring key = jsonb_read_fixed_sized_dstring(rs, key_size, &error);
				if(error)
				{
					delete_jsonb_node(node_p);
					return NULL;
				}

				// parse value as jsonb_node
				jsonb_node* n_p = parse_jsonb(rs);
				if(n_p == NULL)
				{
					delete_jsonb_node(node_p);
					deinit_dstring(&key);
					return NULL;
				}

				if(!put_in_jsonb_object_node2(node_p, key, n_p))
				{
					delete_jsonb_node(node_p);
					deinit_dstring(&key);
					delete_jsonb_node(n_p);
					return NULL;
				}
			}

			node_p->skip_size = skip_size;
			break;
		}
		default : // unidentified type
			return NULL;
	}

	return node_p;
}