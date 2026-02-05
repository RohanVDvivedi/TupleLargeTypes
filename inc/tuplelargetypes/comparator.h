#ifndef BINARY_COMPARATOR_H
#define BINARY_COMPARATOR_H

#include<tuplelargetypes/binary_reader_interface.h>

int compare_tb(const binary_reader_interface* bri1_p, const binary_reader_interface* bri2_p, int* is_prefix);

#define compare_text compare_tb
#define compare_binary compare_tb

#include<tuplelargetypes/numeric_reader_interface.h>

int compare_numeric(const numeric_reader_interface* nri1_p, const numeric_reader_interface* nri2_p, int* is_prefix);

/*
	if first parameter is prefix of another then (is_prefix & 1) returns true
	if second parameter is prefix of another then (is_prefix & 2) returns true
	if both are equal strings or binarys, the both the above conditions return true
	is_prefix will only be set if both the parameters are not NULL
*/

#endif