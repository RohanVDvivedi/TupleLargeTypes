#include<tuplelargetypes/tuple_list_helper.h>

#include<stdlib.h>

typedef struct get_size_context get_size_context;
struct get_size_context
{
	binary_read_iterator* bri_p;
	const void* transaction_id;
	int* abort_error;
};

static uint32_t read_tuple_prefix_from_iterator(void* context_p, void* data, uint32_t data_size)
{
	if(data_size == 0) // this will never happen
		return 0;

	get_size_context* gsc_p = context_p;

	{
		uint32_t bytes_peeked = 0;
		const void* tuple_prefix = peek_in_binary_read_iterator(gsc_p->bri_p, &bytes_peeked, gsc_p->transaction_id, gsc_p->abort_error);
		if(*(gsc_p->abort_error))
			return 0;
		if(bytes_peeked >= data_size)
		{
			memory_move(data, tuple_prefix, data_size);
			return data_size;
		}
	}

	// else if too few bytes are peeked

	// then close the iterator and make read call from the cloned iterator

	binary_read_iterator* cloned_bri_p = clone_binary_read_iterator(gsc_p->bri_p, gsc_p->transaction_id, gsc_p->abort_error);
	if(*(gsc_p->abort_error))
		return 0;

	uint32_t bytes_read = read_from_binary_read_iterator(cloned_bri_p, data, data_size, gsc_p->transaction_id, gsc_p->abort_error);
	if(*(gsc_p->abort_error))
	{
		delete_binary_read_iterator(cloned_bri_p, gsc_p->transaction_id, gsc_p->abort_error);
		return 0;
	}

	delete_binary_read_iterator(cloned_bri_p, gsc_p->transaction_id, gsc_p->abort_error);
	return bytes_read;
}

// the passed buffer must be atleast 4 bytes big, buffer_size will be set to bytes read from the bri_p
uint32_t get_curr_tuple_size_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error)
{
	get_size_context gsc = {.bri_p = bri_p, .transaction_id = transaction_id, abort_error};
	uint32_t curr_tuple_size = get_tuple_size2(tpl_d, &gsc, read_tuple_prefix_from_iterator);
	if(*abort_error)
		return 0;
	return curr_tuple_size;
}

int skip_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error)
{
	// get tuple_size of the tuple we are pointing to right now
	const uint32_t tuple_size = get_curr_tuple_size_from_binary_read_iterator(bri_p, tpl_d, transaction_id, abort_error);
	if(*abort_error)
		return 0;
	if(tuple_size == 0)
		return 0;

	// skip tuple_size number of bytes
	uint32_t bytes_skipped = read_from_binary_read_iterator(bri_p, NULL, tuple_size, transaction_id, abort_error);
	if(*abort_error)
		return 0;
	if(bytes_skipped < tuple_size)
		return 0;

	return 1;
}

const void* peek_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error)
{
	// get tuple_size of the tuple we are pointing to right now
	const uint32_t tuple_size = get_curr_tuple_size_from_binary_read_iterator(bri_p, tpl_d, transaction_id, abort_error);
	if(*abort_error)
		return NULL;
	if(tuple_size == 0)
		return NULL;

	// peek tuple_size number of bytes, if you fall short, fail this call
	uint32_t bytes_peeked = 0;
	const void* tuple = peek_in_binary_read_iterator(bri_p, &bytes_peeked, transaction_id, abort_error);
	if(*abort_error)
		return NULL;
	if(bytes_peeked < tuple_size) // if you are able to peek lesser bytes, fail this call
		return NULL;

	return tuple;
}

void* read_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error)
{
	// get tuple_size of the tuple we are pointing to right now
	const uint32_t tuple_size = get_curr_tuple_size_from_binary_read_iterator(bri_p, tpl_d, transaction_id, abort_error);
	if(*abort_error)
		return NULL;
	if(tuple_size == 0)
		return NULL;

	// malloc for tuple
	void* tuple = malloc(tuple_size);
	if(tuple == NULL)
		exit(-1);

	// read tuple_size number of bytes into it
	uint32_t bytes_read = read_from_binary_read_iterator(bri_p, tuple, tuple_size, transaction_id, abort_error);
	if(*abort_error)
	{
		free(tuple);
		return NULL;
	}
	if(bytes_read < tuple_size)
	{
		free(tuple);
		return NULL;
	}

	return tuple;
}