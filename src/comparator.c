#include<tuplelargetypes/comparator.h>

int compare_tb(binary_read_iterator* bri1_p, binary_read_iterator* bri2_p, int* is_prefix, const void* transaction_id, int* abort_error)
{
	(*is_prefix) = 0;

	// if one of them is NULL
	if((bri1_p->is_null) && (bri2_p->is_null))
		return 0;
	else if(!(bri1_p->is_null) && (bri2_p->is_null))
		return 1;
	else if((bri1_p->is_null) && !(bri2_p->is_null))
		return -1;

	// if both are not NULL

	int cmp = 0;
	while(cmp == 0)
	{
		uint32_t data_size1 = 0;
		const char* data1 = peek_in_binary_read_iterator(bri1_p, &data_size1, transaction_id, abort_error);
		if(*abort_error)
			return 0;

		uint32_t data_size2 = 0;
		const char* data2 = peek_in_binary_read_iterator(bri2_p, &data_size2, transaction_id, abort_error);
		if(*abort_error)
			return 0;

		// if one or both of them does not have any more bytes, process those cases first
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
		}
		else if(data_size1 > 0 && data_size2 == 0)
		{
			(*is_prefix) = 2;
			cmp = 1;
		}
		else // process same number of bytes from both of them, if both of them have some number of bytes
		{
			const uint32_t bytes_processed = min(data_size1, data_size2);
			cmp = memory_compare(data1, data2, bytes_processed);

			// consume bytes processed from both of the iterators

			read_from_binary_read_iterator(bri1_p, NULL, bytes_processed, transaction_id, abort_error);
			if(*abort_error)
				return 0;

			read_from_binary_read_iterator(bri2_p, NULL, bytes_processed, transaction_id, abort_error);
			if(*abort_error)
				return 0;
		}
	}

	return cmp;
}

#define BUFFER_CAPACITY 1024

int compare_numeric(const numeric_reader_interface* nri1_p, const numeric_reader_interface* nri2_p, int* is_prefix, int* error)
{
	(*is_prefix) = 0;

	int is_null1 = nri1_p->is_null(nri1_p);
	int is_null2 = nri2_p->is_null(nri2_p);

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
		nri1_p->extract_sign_bits_and_exponent(nri1_p, &sign_bits1, &exponent1);

		numeric_sign_bits sign_bits2;
		int16_t exponent2;
		nri2_p->extract_sign_bits_and_exponent(nri2_p, &sign_bits2, &exponent2);

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

	int cmp = 0;
	while(cmp == 0)
	{
		if(data_size1 == 0) // if there are no more bytes in the data1 then read it
		{
			data1 = buffer1;
			data_size1 = nri1_p->read_digits_as_stream(nri1_p, data1, BUFFER_CAPACITY/sizeof(uint64_t), error);
			if(*error)
			{
				nri1_p->close_digits_stream(nri1_p);
				nri2_p->close_digits_stream(nri2_p);
				return -2;
			}
		}

		if(data_size2 == 0) // if there are no more bytes in the data2 then read it
		{
			data2 = buffer2;
			data_size2 = nri2_p->read_digits_as_stream(nri2_p, data2, BUFFER_CAPACITY/sizeof(uint64_t), error);
			if(*error)
			{
				nri1_p->close_digits_stream(nri1_p);
				nri2_p->close_digits_stream(nri2_p);
				return -2;
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
		}
		else if(data_size1 > 0 && data_size2 == 0)
		{
			(*is_prefix) = 2;
			cmp = 1;
		}
		else
		{
			const uint32_t digits_processed = min(data_size1, data_size2);
			for(uint32_t i = 0; i < digits_processed && cmp == 0; i++)
				cmp = compare_numbers(data1[i], data2[i]);

			data1 += digits_processed;
			data2 += digits_processed;
			data_size1 -= digits_processed;
			data_size2 -= digits_processed;
		}
	}

	nri1_p->close_digits_stream(nri1_p);
	nri2_p->close_digits_stream(nri2_p);

	return cmp * digits_requirement;
}