#ifndef BINARY_COMPARATOR_H
#define BINARY_COMPARATOR_H

#include<tuple_def.h>
#include<tuple.h>

#include<worm.h>
#include<page_access_methods.h>

int compare_tb(const tuple_def* tpl_d1, const void* tupl1, positional_accessor inline_accessor1, const worm_tuple_defs* wtd1_p, const page_access_methods* pam1_p, const void* transaction_id1, int* abort_error1,
				const tuple_def* tpl_d2, const void* tupl2, positional_accessor inline_accessor2, const worm_tuple_defs* wtd2_p, const page_access_methods* pam2_p, const void* transaction_id2, int* abort_error2,
				int* is_prefix);

#define compare_text compare_tb
#define compare_blob compare_tb

int compare_numeric(const tuple_def* tpl_d1, const void* tupl1, positional_accessor inline_accessor1, const worm_tuple_defs* wtd1_p, const page_access_methods* pam1_p, const void* transaction_id1, int* abort_error1,
					const tuple_def* tpl_d2, const void* tupl2, positional_accessor inline_accessor2, const worm_tuple_defs* wtd2_p, const page_access_methods* pam2_p, const void* transaction_id2, int* abort_error2,
					int* is_prefix);

/*
	if first parameter is prefix of another then (is_prefix & 1) returns true
	if second parameter is prefix of another then (is_prefix & 2) returns true
	if both are equal strings or blobs, the both the above conditions return true
	is_prefix will only be set if both the parameters are not NULL
*/

#endif