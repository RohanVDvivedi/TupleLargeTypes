#ifndef BINARY_WRITER_STREAM_H
#define BINARY_WRITER_STREAM_H

/*
	provides a wrapper stream over the binary_write_iterator
	you the user needs to provide a binary_write_iterator, transaction_id and an abort_error pointer
	An error of BINARY_WRITER_OVERFLOW is thrown if bytes can not be written to it
*/

#include<cutlery/stream.h>
#include<tuplelargetypes/binary_write_iterator.h>

#define BINARY_WRITER_OVERFLOW -177 // chosen arbitrarily

typedef struct binary_writer_stream_context binary_writer_stream_context;
struct binary_writer_stream_context
{
	binary_write_iterator* bwi_p;
	const void* transaction_id;
	int* abort_error;
};

int initialize_stream_for_binary_write_iterator(stream* strm, binary_writer_stream_context* bwisc_p, uint32_t write_buffer_size);

#define initialize_stream_for_binary_write_iterator_static(strm, bwi_p_v, transaction_id_v, abort_error_v, write_buffer_size) \
	initialize_stream_for_binary_write_iterator(strm, &(binary_writer_stream_context){bwi_p_v, transaction_id_v, abort_error_v}, write_buffer_size)

#endif