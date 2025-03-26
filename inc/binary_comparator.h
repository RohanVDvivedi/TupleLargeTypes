#ifndef BINARY_COMPARATOR_H
#define BINARY_COMPARATOR_H

#include<tuple_def.h>
#include<tuple.h>

#include<worm.h>
#include<page_access_methods.h>

int compare_tb(const tuple_def* tpl_d1, const void* tupl1, positional_accessor inline_accessor1, const worm_tuple_defs* wtd1_p, const page_access_methods* pam1_p, const void* transaction_id1, int* abort_error1,
				const tuple_def* tpl_d2, const void* tupl2, positional_accessor inline_accessor2, const worm_tuple_defs* wtd2_p, const page_access_methods* pam2_p, const void* transaction_id2, int* abort_error2);

#define compare_text compare_tb
#define compare_blob compare_tb

#endif