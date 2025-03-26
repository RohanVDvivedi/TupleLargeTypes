#include<binary_comparator.h>

#include<common_extended.h>
#include<relative_positional_accessor.h>

#define BUFFER_CAPACITY 1024

int compare_tb(const tuple_def* tpl_d1, const void* tupl1, positional_accessor inline_accessor1, const worm_tuple_defs* wtd1_p, const page_access_methods* pam1_p, const void* transaction_id1, int* abort_error1,
				const tuple_def* tpl_d2, const void* tupl2, positional_accessor inline_accessor2, const worm_tuple_defs* wtd2_p, const page_access_methods* pam2_p, const void* transaction_id2, int* abort_error2)
{
	int exists1;
	{
		user_value uval1;
		int valid1 = get_value_from_element_from_tuple(&uval1, tpl_d1, inline_accessor1, tupl1);
		exists1 = (valid1 && !is_user_value_NULL(&uval1));
	}

	int exists2;
	{
		user_value uval2;
		int valid2 = get_value_from_element_from_tuple(&uval2, tpl_d2, inline_accessor2, tupl2);
		exists2 = (valid2 && !is_user_value_NULL(&uval2));
	}

	// if one of them does not exists
	if(!exists1 && !exists2)
		return 0;
	else if(exists1 && !exists2)
		return 1;
	else if(!exists1 && exists2)
		return -1;

	// if both exists
	// TODO
}