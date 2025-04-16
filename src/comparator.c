#include<tuplelargetypes/comparator.h>

int compare_tb(const binary_reader_interface* bri1_p, const binary_reader_interface* bri2_p, int* is_prefix)
{
	(*is_prefix) = 0;

	// if any one if it is not valid return -2
	if((!bri1_p->is_valid(bri1_p)) || (!bri2_p->is_valid(bri2_p)))
		return -2;

	int is_null1 = bri1_p->is_null(bri1_p);
	int is_null2 = bri2_p->is_null(bri2_p);

	// if one of them is NULL
	if(is_null1 && is_null2)
		return 0;
	else if(!is_null1 && is_null2)
		return 1;
	else if(is_null1 && !is_null2)
		return -1;

	// if both are not NULL

	uint32_t buffer_size1 = 0;
	uint32_t buffer_size2 = 0;

	const char* data1 = NULL; uint32_t data_size1 = 0;
	const char* data2 = NULL; uint32_t data_size2 = 0;

	int cmp = 0;
	while(cmp == 0)
	{
		if(data_size1 == 0) // if there are no more bytes in the data1 then read it
		{
			int error = 0;

			// skip the bytes processed from the previous peek
			bri1_p->read_bytes_as_stream(bri1_p, NULL, buffer_size1, &error);
			if(error)
				goto ON_ERROR1;

			// peek new bytes
			data1 = bri1_p->peek_bytes_as_stream(bri1_p, &data_size1, &error);
			if(error)
				goto ON_ERROR1;
			// set bytes to be skipped next
			buffer_size1 = data_size1;

			ON_ERROR1:;
			if(error)
			{
				bri1_p->close_bytes_stream(bri1_p);
				bri2_p->close_bytes_stream(bri2_p);
				return 0;
			}
		}

		if(data_size2 == 0) // if there are no more bytes in the data2 then read it
		{
			int error = 0;

			// skip the bytes processed from the previous peek
			bri2_p->read_bytes_as_stream(bri2_p, NULL, buffer_size2, &error);
			if(error)
				goto ON_ERROR2;

			// peek new bytes
			data2 = bri2_p->peek_bytes_as_stream(bri2_p, &data_size2, &error);
			if(error)
				goto ON_ERROR2;
			// set bytes to be skipped next
			buffer_size2 = data_size2;

			ON_ERROR2:;
			if(error)
			{
				bri1_p->close_bytes_stream(bri1_p);
				bri2_p->close_bytes_stream(bri2_p);
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

	bri1_p->close_bytes_stream(bri1_p);
	bri2_p->close_bytes_stream(bri2_p);
	return cmp;
}

#define BUFFER_CAPACITY 1024

int compare_numeric(const numeric_reader_interface* nri1_p, const numeric_reader_interface* nri2_p, int* is_prefix)
{
	(*is_prefix) = 0;

	// if any one if it is not valid return -2
	if((!nri1_p->is_valid(nri1_p)) || (!nri2_p->is_valid(nri2_p)))
		return -2;

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
			int error = 0;
			data1 = buffer1;
			data_size1 = nri1_p->read_digits_as_stream(nri1_p, data1, BUFFER_CAPACITY/sizeof(uint64_t), &error);
			if(error)
			{
				nri1_p->close_digits_stream(nri1_p);
				nri2_p->close_digits_stream(nri2_p);
				return 0;
			}
		}

		if(data_size2 == 0) // if there are no more bytes in the data2 then read it
		{
			int error = 0;
			data2 = buffer2;
			data_size2 = nri2_p->read_digits_as_stream(nri2_p, data2, BUFFER_CAPACITY/sizeof(uint64_t), &error);
			if(error)
			{
				nri1_p->close_digits_stream(nri1_p);
				nri2_p->close_digits_stream(nri2_p);
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

	nri1_p->close_digits_stream(nri1_p);
	nri2_p->close_digits_stream(nri2_p);

	return cmp * digits_requirement;
}