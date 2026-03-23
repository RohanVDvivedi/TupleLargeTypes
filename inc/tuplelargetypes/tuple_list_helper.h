#ifndef TUPLE_LIST_HELPER_H
#define TUPLE_LIST_HELPER_H

#include<tuplelargetypes/binary_read_iterator.h>

// this function returns the size of the tuple that this iterator is pointing to
uint32_t get_curr_tuple_size_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error);

// returns 1 if a tuple was skipped, else returns 0 for abort_error OR end of data
int skip_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error);

// first try peeking, if the tuple you are pointing to fits in the chunk itself, then a pointer to the tuple in the chunk is returned
// you do not need to free this pointer but you will need to skip this tuple after use of this pointer
const void* peek_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error);

// then try reading it, this function does a force memcpy of the tuple from the iterator into a newlu malloc-ed buffer
// you will need to free this buffer, the tuple returned by this function is already skipped
void* read_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error);

#endif