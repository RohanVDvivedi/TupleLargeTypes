#include<tuplelargetypes/jsonb_reader_interface.h>

/*
	implementation for reading from a binary_read_iterator
*/

uint32_t jsonb_from_binary_read_iterator_read_jsonb_bytes(const jsonb_reader_interface* jri_p, char* bytes, uint32_t bytes_size, int* error)
{
	// no bytes to read
	if(bytes_size == 0)
		return 0;

	jsonb_from_binary_read_iterator_context* cntxt = jri_p->context;

	uint32_t bytes_read = 0;
	while(bytes_read < bytes_size)
	{
		uint32_t bytes_read_this_iteration = read_from_binary_read_iterator(cntxt->bri_p, bytes + bytes_read, bytes_size - bytes_read, cntxt->transaction_id, cntxt->abort_error);
		if((*(cntxt->abort_error)))
		{
			(*error) = (*cntxt->abort_error);
			return 0;
		}

		if(bytes_read_this_iteration == 0)
			break;

		bytes_read += bytes_read_this_iteration;
	}

	return bytes_read;
}