#include<tuplelargetypes/jsonb_writer_interface.h>

/*
	implementation for writing to a binary_write_iterator
*/

void jsonb_to_binary_write_iterator_write_jsonb_bytes(jsonb_writer_interface* jwi_p, const char* bytes, uint32_t bytes_size, int* error)
{
	// no bytes to write
	if(bytes_size == 0)
		return;

	jsonb_to_binary_write_iterator_context* cntxt = jwi_p->context;

	uint32_t bytes_written = 0;
	while(bytes_written < bytes_size)
	{
		uint32_t bytes_written_this_iteration = append_to_binary_write_iterator(cntxt->bwi_p, bytes + bytes_written, bytes_size - bytes_written, cntxt->transaction_id, cntxt->abort_error);

		if((*(cntxt->abort_error)))
		{
			(*error) = (*cntxt->abort_error);
			return;
		}

		bytes_written += bytes_written_this_iteration;
	}
}