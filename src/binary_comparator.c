#include<tuplelargetypes/binary_comparator.h>

#include<tuplelargetypes/common_extended.h>
#include<tuplelargetypes/relative_positional_accessor.h>

#include<tuplelargetypes/binary_read_iterator.h>

#define BUFFER_CAPACITY 1024

int compare_tb(const tuple_def* tpl_d1, const void* tupl1, positional_accessor inline_accessor1, const worm_tuple_defs* wtd1_p, const page_access_methods* pam1_p, const void* transaction_id1, int* abort_error1,
				const tuple_def* tpl_d2, const void* tupl2, positional_accessor inline_accessor2, const worm_tuple_defs* wtd2_p, const page_access_methods* pam2_p, const void* transaction_id2, int* abort_error2,
				int* is_prefix)
{
	(*is_prefix) = 0;

	int is_null1;
	{
		user_value uval1;
		if(!get_value_from_element_from_tuple(&uval1, tpl_d1, inline_accessor1, tupl1))
			return -2;
		is_null1 = is_user_value_NULL(&uval1);
	}

	int is_null2;
	{
		user_value uval2;
		if(!get_value_from_element_from_tuple(&uval2, tpl_d2, inline_accessor2, tupl2))
			return -2;
		is_null2 = is_user_value_NULL(&uval2);
	}

	// if one of them is NULL
	if(is_null1 && is_null2)
		return 0;
	else if(!is_null1 && is_null2)
		return 1;
	else if(is_null1 && !is_null2)
		return -1;

	// if both are not NULL

	char buffer1[BUFFER_CAPACITY];
	char buffer2[BUFFER_CAPACITY];

	char* data1 = buffer1; uint32_t data_size1 = 0;
	char* data2 = buffer2; uint32_t data_size2 = 0;

	binary_read_iterator* bri1_p = get_new_binary_read_iterator(tupl1, tpl_d1, inline_accessor1, wtd1_p, pam1_p);
	binary_read_iterator* bri2_p = get_new_binary_read_iterator(tupl2, tpl_d2, inline_accessor2, wtd2_p, pam2_p);

	int cmp = 0;
	while(cmp == 0)
	{
		if(data_size1 == 0) // if there are no more bytes in the data1 then read it
		{
			data1 = buffer1;
			data_size1 = read_from_binary_read_iterator(bri1_p, data1, BUFFER_CAPACITY, transaction_id1, abort_error1);
			if(*abort_error1)
			{
				delete_binary_read_iterator(bri1_p, transaction_id1, abort_error1);
				delete_binary_read_iterator(bri2_p, transaction_id2, abort_error2);
				return 0;
			}
		}

		if(data_size2 == 0) // if there are no more bytes in the data2 then read it
		{
			data2 = buffer2;
			data_size2 = read_from_binary_read_iterator(bri2_p, data2, BUFFER_CAPACITY, transaction_id2, abort_error2);
			if(*abort_error2)
			{
				delete_binary_read_iterator(bri1_p, transaction_id1, abort_error1);
				delete_binary_read_iterator(bri2_p, transaction_id2, abort_error2);
				return 0;
			}
		}

		// if one or both of them does not have any more bytes
		if(data_size1 == 0 && data_size2 == 0) // both of them are prefixes of each other and are equal
		{
			(*is_prefix) = 1 | 2;
			cmp = 0;
			break;
		}
		// else shorter one is the prefix of the another
		else if(data_size1 == 0 && data_size2 > 0)
		{
			(*is_prefix) = 1;
			cmp = -1;
			break;
		}
		else if(data_size1 > 0 && data_size2 == 0)
		{
			(*is_prefix) = 2;
			cmp = 1;
			break;
		}

		const uint32_t bytes_processed = min(data_size1, data_size2);
		cmp = memory_compare(data1, data2, bytes_processed);

		data1 += bytes_processed;
		data2 += bytes_processed;
		data_size1 -= bytes_processed;
		data_size2 -= bytes_processed;
	}

	delete_binary_read_iterator(bri1_p, transaction_id1, abort_error1);
	delete_binary_read_iterator(bri2_p, transaction_id2, abort_error2);
	return cmp;
}

#include<tuplelargetypes/numeric_extended.h>
#include<tuplelargetypes/digit_read_iterator.h>

int compare_numeric(const tuple_def* tpl_d1, const void* tupl1, positional_accessor inline_accessor1, const worm_tuple_defs* wtd1_p, const page_access_methods* pam1_p, const void* transaction_id1, int* abort_error1,
					const tuple_def* tpl_d2, const void* tupl2, positional_accessor inline_accessor2, const worm_tuple_defs* wtd2_p, const page_access_methods* pam2_p, const void* transaction_id2, int* abort_error2,
					int* is_prefix)
{
	(*is_prefix) = 0;

	int is_null1;
	{
		user_value uval1;
		if(!get_value_from_element_from_tuple(&uval1, tpl_d1, inline_accessor1, tupl1))
			return -2;
		is_null1 = is_user_value_NULL(&uval1);
	}

	int is_null2;
	{
		user_value uval2;
		if(!get_value_from_element_from_tuple(&uval2, tpl_d2, inline_accessor2, tupl2))
			return -2;
		is_null2 = is_user_value_NULL(&uval2);
	}

	// if one of them is NULL
	if(is_null1 && is_null2)
		return 0;
	else if(!is_null1 && is_null2)
		return 1;
	else if(is_null1 && !is_null2)
		return -1;

	int digits_requirement = 0;

	// compare first with sign bits and exponent
	{
		numeric_sign_bits sign_bits1;
		int16_t exponent1;
		if(!extract_sign_bits_and_exponent_from_numeric(&sign_bits1, &exponent1, tupl1, tpl_d1, inline_accessor1))
			return -2;

		numeric_sign_bits sign_bits2;
		int16_t exponent2;
		if(!extract_sign_bits_and_exponent_from_numeric(&sign_bits2, &exponent2, tupl2, tpl_d2, inline_accessor2))
			return -2;

		int cmp = compare_numeric_prefix_no_digits(sign_bits1, exponent1, sign_bits2, exponent2, &digits_requirement);

		if(!digits_requirement) // if digits are not required for comparison then early quit
		{
			if(cmp == 0) // at this place here if they are equal then both are prefixes or each other
				(*is_prefix) = 1 | 2;
			return cmp;
		}
	}


	// compare using digits
	uint64_t buffer1[BUFFER_CAPACITY/sizeof(uint64_t)];
	uint64_t buffer2[BUFFER_CAPACITY/sizeof(uint64_t)];

	uint64_t* data1 = buffer1; uint32_t data_size1 = 0;
	uint64_t* data2 = buffer2; uint32_t data_size2 = 0;

	digit_read_iterator* dri1_p = get_new_digit_read_iterator(tupl1, tpl_d1, inline_accessor1, wtd1_p, pam1_p);
	digit_read_iterator* dri2_p = get_new_digit_read_iterator(tupl2, tpl_d2, inline_accessor2, wtd2_p, pam2_p);

	int cmp = 0;
	while(cmp == 0)
	{
		if(data_size1 == 0) // if there are no more bytes in the data1 then read it
		{
			data1 = buffer1;
			data_size1 = read_from_digit_read_iterator(dri1_p, data1, BUFFER_CAPACITY/sizeof(uint64_t), transaction_id1, abort_error1);
			if(*abort_error1)
			{
				delete_digit_read_iterator(dri1_p, transaction_id1, abort_error1);
				delete_digit_read_iterator(dri2_p, transaction_id2, abort_error2);
				return 0;
			}
		}

		if(data_size2 == 0) // if there are no more bytes in the data2 then read it
		{
			data2 = buffer2;
			data_size2 = read_from_digit_read_iterator(dri2_p, data2, BUFFER_CAPACITY/sizeof(uint64_t), transaction_id2, abort_error2);
			if(*abort_error2)
			{
				delete_digit_read_iterator(dri1_p, transaction_id1, abort_error1);
				delete_digit_read_iterator(dri2_p, transaction_id2, abort_error2);
				return 0;
			}
		}

		// if one or both of them does not have any more digits
		if(data_size1 == 0 && data_size2 == 0) // both of them are prefixes of each other and are equal
		{
			(*is_prefix) = 1 | 2;
			cmp = 0;
			break;
		}
		// else shorter one is the prefix of the another
		else if(data_size1 == 0 && data_size2 > 0)
		{
			(*is_prefix) = 1;
			cmp = -1;
			break;
		}
		else if(data_size1 > 0 && data_size2 == 0)
		{
			(*is_prefix) = 2;
			cmp = 1;
			break;
		}

		const uint32_t digits_processed = min(data_size1, data_size2);
		for(uint32_t i = 0; i < digits_processed && cmp == 0; i++)
			cmp = compare_numbers(data1[i], data2[i]);

		data1 += digits_processed;
		data2 += digits_processed;
		data_size1 -= digits_processed;
		data_size2 -= digits_processed;
	}

	delete_digit_read_iterator(dri1_p, transaction_id1, abort_error1);
	delete_digit_read_iterator(dri2_p, transaction_id2, abort_error2);

	return cmp * digits_requirement;
}