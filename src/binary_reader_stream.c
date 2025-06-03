#include<tuplelargetypes/binary_reader_stream.h>

static cy_uint read_from_stream_context(void* stream_context, void* data, cy_uint data_size, int* error)
{
	binary_reader_stream_context* brsc_p = stream_context;

	uint32_t data_read = read_from_binary_read_iterator(brsc_p->bri_p, data, min(data_size, UINT32_MAX), brsc_p->transaction_id, brsc_p->abort_error);
	if(*(brsc_p->abort_error))
	{
		(*error) = (*(brsc_p->abort_error));
		return 0;
	}

	return data_read;
}

static void close_stream_context(void* stream_context, int* error)
{
	// NOP
}

static void destroy_stream_context(void* stream_context)
{
	// NOP
}

int initialize_stream_for_binary_read_iterator(stream* strm, binary_reader_stream_context* brsc_p)
{
	if(!initialize_stream(strm, brsc_p, read_from_stream_context, NULL, close_stream_context, destroy_stream_context, NULL, DEFAULT_MAX_UNFLUSHED_BYTES_COUNT))
	{
		// since we never opened this stream_context, so we do not close it
		destroy_stream_context(NULL);
		return 0;
	}

	return 1;
}