#include<tuple_list_helper.h>

// the passed buffer must be atleast 4 bytes big, buffer_size will be set to bytes read from the bri_p
static uint32_t get_next_tuple_size_from_binary_read_iterator(binary_read_iterator* bri_p, char* buffer, uint32_t* buffer_size, const void* transaction_id, int* abort_error)
{
	// TODO
}

int skip_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const void* transaction_id, int* abort_error)
{
	// TODO
}

void* read_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const void* transaction_id, int* abort_error)
{
	// TODO
}