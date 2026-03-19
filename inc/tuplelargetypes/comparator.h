#ifndef BINARY_COMPARATOR_H
#define BINARY_COMPARATOR_H

#include<tuplelargetypes/binary_read_iterator.h>

int compare_tb(binary_read_iterator* bri1_p, binary_read_iterator* bri2_p, int* is_prefix, const void* transaction_id, int* abort_error);

#define compare_text compare_tb
#define compare_blob compare_tb

#include<tuplelargetypes/numeric_reader_interface.h>

int compare_numeric(const numeric_reader_interface* nri1_p, const numeric_reader_interface* nri2_p, int* is_prefix, int* error);

/*
	if first parameter is prefix of another then (is_prefix & 1) returns true
	if second parameter is prefix of another then (is_prefix & 2) returns true
	if both are equal strings or binarys or numerics, the both the above conditions return true
	is_prefix will only be set if both the parameters are not NULL
*/

#endif