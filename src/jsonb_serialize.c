#include<tuplelargetypes/jsonb_serialize.h>

#include<serint/serial_int.h>

static inline void jsonb_writer_interface_write_uint8(const jsonb_writer_interface* jwi_p, uint8_t b, int* error)
{
	char byte = b;
	jwi_p->write_jsonb_bytes(jwi_p, &byte, 1, error);
}

static inline void jsonb_writer_interface_write_uint32(const jsonb_writer_interface* jwi_p, uint32_t b, int* error)
{
	char bytes[4];
	serialize_uint32(bytes, 4, b);
	jwi_p->write_jsonb_bytes(jwi_p, bytes, 4, error);
}

int jsonb_serialize(const jsonb_writer_interface* jwi_p, const jsonb_node* node_p)
{
	int error = 0;

	if(node_p == NULL)
	{
		jsonb_writer_interface_write_uint8(jwi_p, JSONB_NULL, &error);
		goto EXIT;
	}

	jsonb_writer_interface_write_uint8(jwi_p, node_p->type, &error);
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
			jsonb_writer_interface_write_uint32(jwi_p, node_p->skip_size, &error);
			if(error)
				goto EXIT;

			jwi_p->write_jsonb_bytes(jwi_p, get_byte_array_dstring(&(node_p->jsonb_string)), get_char_count_dstring(&(node_p->jsonb_string)), &error);
			if(error)
				goto EXIT;

			break;
		}
		case JSONB_NUMERIC :
		{
			jsonb_writer_interface_write_uint32(jwi_p, node_p->skip_size, &error);
			if(error)
				goto EXIT;

			// TODO
			break;
		}
		case JSONB_ARRAY :
		{
			jsonb_writer_interface_write_uint32(jwi_p, node_p->skip_size, &error);
			if(error)
				goto EXIT;

			jsonb_writer_interface_write_uint32(jwi_p, get_element_count_arraylist(&(node_p->jsonb_array)), &error);
			if(error)
				goto EXIT;

			for(uint32_t i = 0; i < get_element_count_arraylist(&(node_p->jsonb_array)); i++)
			{
				const jsonb_node* n_p = get_from_front_of_arraylist(&(node_p->jsonb_array), i);
				if(!jsonb_serialize(jwi_p, n_p))
					return 0;
			}

			break;
		}
		case JSONB_OBJECT :
		{
			jsonb_writer_interface_write_uint32(jwi_p, node_p->skip_size, &error);
			if(error)
				goto EXIT;

			jsonb_writer_interface_write_uint32(jwi_p, node_p->element_count, &error);
			if(error)
				goto EXIT;

			for(const jsonb_object_entry* e = find_smallest_in_bst(&(node_p->jsonb_object)); e != NULL; e = get_inorder_next_of_in_bst(&(node_p->jsonb_object), e))
			{
				jsonb_writer_interface_write_uint32(jwi_p, get_char_count_dstring(&(e->key)), &error);
				if(error)
					goto EXIT;

				jwi_p->write_jsonb_bytes(jwi_p, get_byte_array_dstring(&(e->key)), get_char_count_dstring(&(e->key)), &error);
				if(error)
					goto EXIT;

				if(!jsonb_serialize(jwi_p, e->value))
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