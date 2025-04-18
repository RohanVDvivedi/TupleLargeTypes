#ifndef BINARY_WRITE_ITERATOR_STREAM_H
#define BINARY_WRITE_ITERATOR_STREAM_H

/*
	provides a wrapper stream over the binary_write_iterator
	you the user needs to provide a binary_write_iterator, transaction_id and an error pointer
	An error of BINARY_WRITER_OVERFLOW is thrown if bytes can not be written to it
*/

#include<cutlery/stream.h>
#include<tuplelargetypes/binary_write_iterator.h>

#define BINARY_WRITER_OVERFLOW -177 // chosen arbitrarily

typedef struct binary_write_iterator_stream_context binary_write_iterator_stream_context;
struct binary_write_iterator_stream_context
{
	binary_write_iterator* bwi_p;
	const void* transaction_id;
	int* abort_error;
};

int initialize_stream_for_binary_write_iterator(stream* strm, binary_write_iterator_stream_context* bwi_p);

#define initialize_stream_for_binary_write_iterator_static(strm, bwi_p_v, transaction_id_v, abort_error_v) \
	initialize_stream_for_binary_write_iterator(strm, &(binary_write_iterator_stream_context){bwi_p_v, transaction_id_v, abort_error_v})

#endif