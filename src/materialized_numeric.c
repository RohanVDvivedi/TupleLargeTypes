#include<tuplelargetypes/materialized_numeric.h>

declarations_value_arraylist(digits_list, uint64_t, static inline)
#define EXPANSION_FACTOR 1.5
function_definitions_value_arraylist(digits_list, uint64_t, static inline)

int initialize_materialized_numeric(materialized_numeric* m, uint32_t digits_capacity)
{
	m->sign_bits = 0;
	m->exponent = 0;
	return initialize_digits_list(&(m->digits), digits_capacity);
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

int can_materialized_numeric_have_exponent_digits(const materialized_numeric* m)
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
	if(!can_materialized_numeric_have_exponent_digits(m))
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
	if(!can_materialized_numeric_have_exponent_digits(m))
		return 0;

	if(push_front_to_digits_list(&(m->digits), &digit))
		return 1;

	expand_digits_list(&(m->digits));
	return push_front_to_digits_list(&(m->digits), &digit);
}

int push_lsd_in_materialized_numeric(materialized_numeric* m, uint64_t digit)
{
	if(!can_materialized_numeric_have_exponent_digits(m))
		return 0;

	if(push_back_to_digits_list(&(m->digits), &digit))
		return 1;

	expand_digits_list(&(m->digits));
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

int compare_materialized_numeric(const materialized_numeric* m1, const materialized_numeric* m2);

void deinitialize_materialized_numeric(materialized_numeric* m)
{
	deinitialize_digits_list(&(m->digits));
}