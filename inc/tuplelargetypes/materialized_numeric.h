#ifndef MATERIALIZED_NUMERIC_H
#define MATERIALIZED_NUMERIC_H

typedef struct materialized_numeric materialized_numeric;

#include<tuplelargetypes/numeric_extended.h>
#include<cutlery/value_arraylist.h>

data_definitions_value_arraylist(digits_list, uint64_t)

/*
	materialized_numeric is a base 10^12 number
*/

struct materialized_numeric
{
	numeric_sign_bits sign_bits;

	int16_t exponent;

	digits_list digits; // each uint64_t is a number between 0 including to 10^12 excluding
};

int initialize_materialized_numeric(materialized_numeric* m, uint32_t digits_capacity);
int initialize_static_materialized_numeric(materialized_numeric* m, numeric_sign_bits sign_bits, int16_t exponent, uint64_t* digits_array, uint32_t digits_count);

// digits_array if provided, makes this a static memory clone, else a suitable buffer is allocated for the digits_list
// digits_capacity is valid only if the digits_array != NULL
int initialize_from_materialized_numeric(materialized_numeric* dest, uint64_t* digits_array, uint32_t digits_capacity, const materialized_numeric* src);

int can_materialized_numeric_have_exponent_and_digits(const materialized_numeric* m);

void get_sign_bits_and_exponent_for_materialized_numeric(const materialized_numeric* m, numeric_sign_bits* sign_bits, int16_t* exponent);
void set_sign_bits_and_exponent_for_materialized_numeric(materialized_numeric* m, numeric_sign_bits sign_bits, int16_t exponent);

uint32_t get_digits_count_for_materialized_numeric(const materialized_numeric* m);

// below two functions fail if you try to append to the -inf, 0 or +inf numerics
int push_msd_in_materialized_numeric(materialized_numeric* m, uint64_t digit);
int push_lsd_in_materialized_numeric(materialized_numeric* m, uint64_t digit);
int pop_lsd_from_materialized_numeric(materialized_numeric* m);

uint64_t get_nth_digit_from_materialized_numeric(const materialized_numeric* m, uint32_t position); // -> get digit at power of (10^12)^(-position)
int set_nth_digit_in_materialized_numeric(materialized_numeric* m, uint64_t digit, uint32_t position);

int compare_materialized_numeric(const materialized_numeric* m1, const materialized_numeric* m2);

void deinitialize_materialized_numeric(materialized_numeric* m);

void print_materialized_numeric(const materialized_numeric* m);

#endif