#include<tuple_list_helper.h>

#include<stdlib.h>

typedef struct get_size_context get_size_context;
struct get_size_context
{
	binary_read_iterator* bri_p;
	const void* transaction_id;
	int* abort_error;
};

static uint32_t read_from_stream_for_get_size_context(void* context_p, void* data, uint32_t data_size)
{
	get_size_context* c_p = context_p;
	return read_from_binary_read_iterator(c_p->bri_p, data, data_size, c_p->transaction_id, c_p->abort_error);
}

// the passed buffer must be atleast 4 bytes big, buffer_size will be set to bytes read from the bri_p
static uint32_t get_next_tuple_size_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, char* buffer, uint32_t* buffer_size, const void* transaction_id, int* abort_error)
{
	(*buffer_size) = 0;
	return get_tuple_size_from_stream(tpl_d, buffer, buffer_size, &(get_size_context){bri_p, transaction_id, abort_error}, read_from_stream_for_get_size_context);
}

int skip_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error)
{
	char buffer[4];
	uint32_t bytes_read = 0;
	const uint32_t bytes_to_read = get_next_tuple_size_from_binary_read_iterator(bri_p, tpl_d, buffer, &bytes_read, transaction_id, abort_error);
	if(*abort_error)
		return 0;

	// end of stream OR error prone data
	if(bytes_to_read == 0)
		return 0;

	// iterate will all of bytes_to_read are not skipped
	while(bytes_read < bytes_to_read)
	{
		uint32_t bytes_read_this_iteration = read_from_binary_read_iterator(bri_p, NULL, bytes_to_read - bytes_read, transaction_id, abort_error);
		if(*abort_error)
			return 0;

		if(bytes_read_this_iteration == 0)
			return 0;

		bytes_read += bytes_read_this_iteration;
	}

	return 1;
}

void* read_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error)
{
	char buffer[4];
	uint32_t bytes_read = 0;
	const uint32_t bytes_to_read = get_next_tuple_size_from_binary_read_iterator(bri_p, tpl_d, buffer, &bytes_read, transaction_id, abort_error);
	if(*abort_error)
		return NULL;

	// end of stream OR error prone data
	if(bytes_to_read == 0)
		return NULL;

	void* tuple = malloc(bytes_to_read);
	memory_move(tuple, buffer, bytes_read);

	// iterate will all of bytes_to_read are not skipped
	while(bytes_read < bytes_to_read)
	{
		uint32_t bytes_read_this_iteration = read_from_binary_read_iterator(bri_p, tuple + bytes_read, bytes_to_read - bytes_read, transaction_id, abort_error);
		if(*abort_error)
		{
			free(tuple);
			return NULL;
		}

		if(bytes_read_this_iteration == 0)
		{
			free(tuple);
			return NULL;
		}

		bytes_read += bytes_read_this_iteration;
	}

	return tuple;
}