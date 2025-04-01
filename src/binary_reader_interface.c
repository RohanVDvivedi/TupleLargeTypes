#include<tuplelargetypes/binary_reader_interface.h>

/*
	implementation for a text/blob inside a tuple (extended or inline)
*/

int is_valid_for_intuple_binary_reader_interface(binary_reader_interface* bri_p)
{
	intuple_binary_reader_interface_context* cntxt = bri_p->context;

	user_value uval;
	int is_valid = get_value_from_element_from_tuple(&uval, cntxt->tpl_d, cntxt->inline_accessor, cntxt->tupl);

	return is_valid;
}

int is_null_for_intuple_binary_reader_interface(binary_reader_interface* bri_p)
{
	intuple_binary_reader_interface_context* cntxt = bri_p->context;

	user_value uval;
	int is_valid = get_value_from_element_from_tuple(&uval, cntxt->tpl_d, cntxt->inline_accessor, cntxt->tupl);
	if(!is_valid) // if not valid, then this value is close to being NULL
		return 1;

	return is_user_value_NULL(&uval);
}

uint32_t read_bytes_as_stream_for_intuple_binary_reader_interface(binary_reader_interface* bri_p, char* data, uint32_t data_size, int* error)
{
	intuple_binary_reader_interface_context* cntxt = bri_p->context;

	if((*(cntxt->abort_error)))
	{
		(*error) = (*cntxt->abort_error);
		return 0;
	}

	if(cntxt->bri_p == NULL)
		cntxt->bri_p = get_new_binary_read_iterator(cntxt->tupl, cntxt->tpl_d, cntxt->inline_accessor, cntxt->wtd_p, cntxt->pam_p);

	data_size = read_from_binary_read_iterator(cntxt->bri_p, data, data_size, cntxt->transaction_id, cntxt->abort_error);
	(*error) = (*cntxt->abort_error);
	if((*(cntxt->abort_error)))
	{
		delete_binary_read_iterator(cntxt->bri_p, cntxt->transaction_id, cntxt->abort_error);
		cntxt->bri_p = NULL;
		return 0;
	}

	return data_size;
}

void close_bytes_stream_for_intuple_binary_reader_interface(binary_reader_interface* bri_p)
{
	intuple_binary_reader_interface_context* cntxt = bri_p->context;
	if(cntxt->bri_p != NULL)
	{
		delete_binary_read_iterator(cntxt->bri_p, cntxt->transaction_id, cntxt->abort_error);
		cntxt->bri_p = NULL;
	}
}

/*
	implementation for a text/blob inside a user_value (extended or inline)
*/

int is_valid_for_user_value_binary_reader_interface(binary_reader_interface* bri_p)
{
	return 1;
}

int is_null_for_user_value_binary_reader_interface(binary_reader_interface* bri_p)
{
	user_value_binary_reader_interface_context* cntxt = bri_p->context;
	return is_user_value_NULL(&(cntxt->uval));
}

uint32_t read_bytes_as_stream_for_user_value_binary_reader_interface(binary_reader_interface* bri_p, char* data, uint32_t data_size, int* error)
{
	user_value_binary_reader_interface_context* cntxt = bri_p->context;

	data_size = min(data_size, cntxt->uval.string_or_blob_size - cntxt->bytes_read);
	memory_move(data, cntxt->uval.string_or_blob_value + cntxt->bytes_read, data_size);
	cntxt->bytes_read += data_size;

	return data_size;
}

void close_bytes_stream_for_user_value_binary_reader_interface(binary_reader_interface* bri_p)
{
}