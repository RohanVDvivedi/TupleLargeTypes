#include<tuplelargetypes/jsonb_serializer.h>

#include<serint/serial_int.h>

static inline void jsonb_write_uint8(stream* ws, uint8_t b, int* error)
{
	char byte = b;
	write_to_stream(ws, &byte, 1, error);
}

static inline void jsonb_write_uint32(stream* ws, uint32_t b, int* error)
{
	char bytes[4];
	serialize_uint32(bytes, 4, b);
	write_to_stream(ws, bytes, 4, error);
}

static inline void jsonb_write_int16(stream* ws, int16_t b, int* error)
{
	char bytes[2];
	serialize_int16(bytes, 2, b);
	write_to_stream(ws, bytes, 2, error);
}

int serialize_jsonb(stream* ws, const jsonb_node* node_p)
{
	int error = 0;

	if(node_p == NULL)
	{
		jsonb_write_uint8(ws, JSONB_NULL, &error);
		goto EXIT;
	}

	jsonb_write_uint8(ws, node_p->type, &error);
	if(error)
		goto EXIT;

	switch(node_p->type)
	{
		case JSONB_NULL :
		case JSONB_TRUE :
		case JSONB_FALSE :
			break;
		case JSONB_STRING :
		{
			jsonb_write_uint32(ws, node_p->skip_size, &error);
			if(error)
				goto EXIT;

			write_to_stream(ws, get_byte_array_dstring(&(node_p->jsonb_string)), get_char_count_dstring(&(node_p->jsonb_string)), &error);
			if(error)
				goto EXIT;

			break;
		}
		case JSONB_NUMERIC :
		{
			jsonb_write_uint32(ws, node_p->skip_size, &error);
			if(error)
				goto EXIT;

			numeric_sign_bits sign_bits;
			int16_t exponent;
			get_sign_bits_and_exponent_for_materialized_numeric(&(node_p->jsonb_numeric), &sign_bits, &exponent);

			jsonb_write_uint8(ws, sign_bits, &error);
			if(error)
				goto EXIT;

			jsonb_write_int16(ws, exponent, &error);
			if(error)
				goto EXIT;

			for(uint32_t i = 0; i < get_digits_count_for_materialized_numeric(&(node_p->jsonb_numeric)); i++)
			{
				uint64_t digit = get_nth_digit_from_materialized_numeric(&(node_p->jsonb_numeric), i);
				char bytes[5];
				serialize_uint64(bytes, 5, digit);
				write_to_stream(ws, bytes, 5, &error);
				if(error)
					goto EXIT;
			}

			break;
		}
		case JSONB_ARRAY :
		{
			jsonb_write_uint32(ws, node_p->skip_size, &error);
			if(error)
				goto EXIT;

			jsonb_write_uint32(ws, get_element_count_arraylist(&(node_p->jsonb_array)), &error);
			if(error)
				goto EXIT;

			for(uint32_t i = 0; i < get_element_count_arraylist(&(node_p->jsonb_array)); i++)
			{
				const jsonb_node* n_p = get_from_front_of_arraylist(&(node_p->jsonb_array), i);
				if(!serialize_jsonb(ws, n_p))
					return 0;
			}

			break;
		}
		case JSONB_OBJECT :
		{
			jsonb_write_uint32(ws, node_p->skip_size, &error);
			if(error)
				goto EXIT;

			jsonb_write_uint32(ws, node_p->element_count, &error);
			if(error)
				goto EXIT;

			for(const jsonb_object_entry* e = find_smallest_in_bst(&(node_p->jsonb_object)); e != NULL; e = get_inorder_next_of_in_bst(&(node_p->jsonb_object), e))
			{
				jsonb_write_uint32(ws, get_char_count_dstring(&(e->key)), &error);
				if(error)
					goto EXIT;

				write_to_stream(ws, get_byte_array_dstring(&(e->key)), get_char_count_dstring(&(e->key)), &error);
				if(error)
					goto EXIT;

				if(!serialize_jsonb(ws, e->value))
					return 0;
			}

			break;
		}
	}

	EXIT:;
	if(error)
		return 0;
	return 1;
}