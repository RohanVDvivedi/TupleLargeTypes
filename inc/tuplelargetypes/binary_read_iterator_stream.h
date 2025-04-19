#ifndef BINARY_READ_ITERATOR_STREAM_H
#define BINARY_READ_ITERATOR_STREAM_H

/*
	provides a wrapper stream over the binary_read_iterator
	you the user needs to provide a binary_read_iterator, transaction_id and an abort_error pointer
*/

#include<cutlery/stream.h>
#include<tuplelargetypes/binary_read_iterator.h>

typedef struct binary_read_iterator_stream_context binary_read_iterator_stream_context;
struct binary_read_iterator_stream_context
{
	binary_read_iterator* bri_p;
	const void* transaction_id;
	int* abort_error;
};

int initialize_stream_for_binary_read_iterator(stream* strm, binary_read_iterator_stream_context* brisc_p);

#define initialize_stream_for_binary_read_iterator_static(strm, bri_p_v, transaction_id_v, abort_error_v) \
	initialize_stream_for_binary_read_iterator(strm, &(binary_read_iterator_stream_context){bri_p_v, transaction_id_v, abort_error_v})

#endif