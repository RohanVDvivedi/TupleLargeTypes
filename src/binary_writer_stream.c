#include<tuplelargetypes/binary_writer_stream.h>

static cy_uint write_to_stream_context(void* stream_context, const void* data, cy_uint data_size, int* error)
{
	binary_write_iterator_stream_context* bwisc_p = stream_context;

	uint32_t data_written = append_to_binary_write_iterator(bwisc_p->bwi_p, data, min(data_size, UINT32_MAX), bwisc_p->transaction_id, bwisc_p->abort_error);

	if(*(bwisc_p->abort_error))
	{
		(*error) = (*(bwisc_p->abort_error));
		return 0;
	}

	if(data_written == 0)
		(*error) = BINARY_WRITER_OVERFLOW;

	return data_written;
}

static void close_stream_context(void* stream_context, int* error)
{
	// NOP
}

static void destroy_stream_context(void* stream_context)
{
	// NOP
}

int initialize_stream_for_binary_write_iterator(stream* strm, binary_write_iterator_stream_context* bwisc_p, uint32_t write_buffer_size)
{
	if(!initialize_stream(strm, bwisc_p, NULL, write_to_stream_context, close_stream_context, destroy_stream_context, NULL, write_buffer_size))
	{
		// since we never opened this stream_context, so we do not close it
		destroy_stream_context(NULL);
		return 0;
	}

	return 1;
}