#include<tuplelargetypes/materialized_numeric.h>

declarations_value_arraylist(digits_list, uint64_t, static inline)
#define EXPANSION_FACTOR 1.5
function_definitions_value_arraylist(digits_list, uint64_t, static inline)

#include<stdlib.h>

int initialize_materialized_numeric(materialized_numeric* m, uint32_t digits_capacity)
{
	m->sign_bits = 0;
	m->exponent = 0;
	if(!initialize_digits_list(&(m->digits), digits_capacity))
		exit(-1);
	return 1;
}

int initialize_static_materialized_numeric(materialized_numeric* m, numeric_sign_bits sign_bits, int16_t exponent, uint64_t* digits_array, uint32_t digits_count)
{
	m->sign_bits = 0;
	m->exponent = 0;
	initialize_digits_list_with_memory(&(m->digits), digits_count, digits_array);
	m->digits.first_index = 0;
	m->digits.element_count = digits_count;
	set_sign_bits_and_exponent_for_materialized_numeric(m, sign_bits, exponent);
	return 1;
}

int initialize_from_materialized_numeric(materialized_numeric* dest, uint64_t* digits_array, uint32_t digits_capacity, const materialized_numeric* src)
{
	if(digits_array == NULL)
	{
		if(!initialize_digits_list(&(dest->digits), get_digits_count_for_materialized_numeric(src)))
			exit(-1);
	}
	else
	{
		if(digits_capacity < get_digits_count_for_materialized_numeric(src))
			return 0;
		if(!initialize_digits_list_with_memory(&(dest->digits), digits_capacity, digits_array))
			exit(-1);
	}
	dest->sign_bits = src->sign_bits;
	dest->exponent = src->exponent;
	for(uint32_t i = 0; i < get_digits_count_for_materialized_numeric(src); i++)
		push_lsd_in_materialized_numeric(dest, get_nth_digit_from_materialized_numeric(src, i));
	return 1;
}

int can_materialized_numeric_have_exponent_and_digits(const materialized_numeric* m)
{
	switch(m->sign_bits)
	{
		case POSITIVE_NUMERIC :
		case NEGATIVE_NUMERIC :
			return 1;
		default : // -inf, +inf and 0 can not have digits
			return 0;
	}
}

void get_sign_bits_and_exponent_for_materialized_numeric(const materialized_numeric* m, numeric_sign_bits* sign_bits, int16_t* exponent)
{
	(*sign_bits) = m->sign_bits;
	(*exponent) = m->exponent;
}

void set_sign_bits_and_exponent_for_materialized_numeric(materialized_numeric* m, numeric_sign_bits sign_bits, int16_t exponent)
{
	if(sign_bits == NEGATIVE_INFINITY_NUMERIC || sign_bits == ZERO_NUMERIC || sign_bits == POSITIVE_INFINITY_NUMERIC)
	{
		exponent = 0;
		remove_all_from_digits_list(&(m->digits));
	}

	m->sign_bits = sign_bits;
	m->exponent = exponent;
}

uint32_t get_digits_count_for_materialized_numeric(const materialized_numeric* m)
{
	return get_element_count_digits_list(&(m->digits));
}

int push_msd_in_materialized_numeric(materialized_numeric* m, uint64_t digit)
{
	if(get_element_count_digits_list(&(m->digits)) == UINT32_MAX)
		return 0;

	if(!can_materialized_numeric_have_exponent_and_digits(m))
		return 0;

	if(push_front_to_digits_list(&(m->digits), &digit))
		return 1;

	if(!expand_digits_list(&(m->digits))) // expand fails -> allocation fails -> exit(-1)
		exit(-1);

	return push_front_to_digits_list(&(m->digits), &digit);
}

int push_lsd_in_materialized_numeric(materialized_numeric* m, uint64_t digit)
{
	if(get_element_count_digits_list(&(m->digits)) == UINT32_MAX)
		return 0;

	if(!can_materialized_numeric_have_exponent_and_digits(m))
		return 0;

	if(push_back_to_digits_list(&(m->digits), &digit))
		return 1;

	if(!expand_digits_list(&(m->digits))) // expand fails -> allocation fails -> exit(-1)
		exit(-1);

	return push_back_to_digits_list(&(m->digits), &digit);
}

int pop_lsd_from_materialized_numeric(materialized_numeric* m)
{
	return pop_back_from_digits_list(&(m->digits));
}

uint64_t get_nth_digit_from_materialized_numeric(const materialized_numeric* m, uint32_t position)
{
	uint64_t digit = 0;
	const uint64_t* d = get_from_front_of_digits_list(&(m->digits), position);
	if(d != NULL)
		digit = (*d);
	return digit;
}

int set_nth_digit_in_materialized_numeric(materialized_numeric* m, uint64_t digit, uint32_t position)
{
	return set_from_front_in_digits_list(&(m->digits), &digit, position);
}

int compare_materialized_numeric(const materialized_numeric* m1, const materialized_numeric* m2)
{
	numeric_sign_bits sign_bits1; int16_t exponent1;
	get_sign_bits_and_exponent_for_materialized_numeric(m1, &sign_bits1, &exponent1);

	numeric_sign_bits sign_bits2; int16_t exponent2;
	get_sign_bits_and_exponent_for_materialized_numeric(m2, &sign_bits2, &exponent2);

	int digits_requirement = 0;
	int cmp = compare_numeric_prefix_no_digits(sign_bits1, exponent1, sign_bits2, exponent2, &digits_requirement);

	if(!digits_requirement)
		return cmp;

	uint32_t digits_count1 = get_digits_count_for_materialized_numeric(m1);
	uint32_t digits_count2 = get_digits_count_for_materialized_numeric(m2);

	uint32_t digits_to_compare = min(digits_count1, digits_count2);
	for(uint32_t i = 0; i < digits_to_compare && cmp == 0; i++)
	{
		uint64_t d1 = get_nth_digit_from_materialized_numeric(m1, i);
		uint64_t d2 = get_nth_digit_from_materialized_numeric(m2, i);
		cmp = compare_numbers(d1, d2);
	}

	if(digits_count1 != digits_count2)
	{
		if(digits_count1 > digits_count2)
			cmp = 1;
		else
			cmp = -1;
	}

	return cmp * digits_requirement;
}

void deinitialize_materialized_numeric(materialized_numeric* m)
{
	deinitialize_digits_list(&(m->digits));
}

void print_materialized_numeric(const materialized_numeric* m)
{
	switch(m->sign_bits)
	{
		case NEGATIVE_INFINITY_NUMERIC :
		{
			printf("-INF");
			return;
		}
		case NEGATIVE_NUMERIC :
		{
			printf("-");
			break;
		}
		case ZERO_NUMERIC :
		{
			printf("0");
			return;
		}
		case POSITIVE_NUMERIC :
		{
			printf("+");
			break;
		}
		case POSITIVE_INFINITY_NUMERIC :
		{
			printf("+INF");
			return;
		}
	}

	for(uint32_t i = 0; i < get_digits_count_for_materialized_numeric(m); i++)
	{
		if(i > 0)
			printf(" ");
		printf("%012"PRIu64, get_nth_digit_from_materialized_numeric(m, i));
		if(i == 0 && get_digits_count_for_materialized_numeric(m) > 1)
			printf(".");
	}

	if(m->exponent != 0)
		printf(" (10^12)^(%"PRId16")", m->exponent);
}