#ifndef TUPLE_LIST_HELPER_H
#define TUPLE_LIST_HELPER_H

#include<tuplelargetypes/binary_read_iterator.h>

// returns 1 if a tuple was skipped, else returns 0 for abort_error OR end of data
int skip_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, const void* transaction_id, int* abort_error);

// returns a pointer to a next tuple read (free_the_pointer = 1) OR peeked (free_the_pointer = 0) from the iterator, else returns NULL for abort_error OR end of data
void* get_tuple_from_binary_read_iterator(binary_read_iterator* bri_p, const tuple_def* tpl_d, int* free_the_pointer, const void* transaction_id, int* abort_error);

#endif