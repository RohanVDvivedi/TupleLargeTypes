#include<tuple_list_helper.h>

#include<stdlib.h>

// the passed buffer must be atleast 4 bytes big, buffer_size will be set to bytes read from the bri_p
static uint32_t get_next_tuple_size_from_binary_read_iterator(binary_read_iterator* bri_p, char* buffer, uint32_t* buffer_size, const void* transaction_id, int* abort_error)
{
	// TODO
}

int skip_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const void* transaction_id, int* abort_error)
{
	char buffer[4];
	uint32_t bytes_read = 0;
	const uint32_t bytes_to_read = get_next_tuple_size_from_binary_read_iterator(bri_p, buffer, &bytes_read, transaction_id, abort_error);
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

void* read_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const void* transaction_id, int* abort_error)
{
	char buffer[4];
	uint32_t bytes_read = 0;
	const uint32_t bytes_to_read = get_next_tuple_size_from_binary_read_iterator(bri_p, buffer, &bytes_read, transaction_id, abort_error);
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