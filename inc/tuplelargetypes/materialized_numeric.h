#ifndef MATERIALIZED_NUMERIC_H
#define MATERIALIZED_NUMERIC_H

typedef struct materialized_numeric materialized_numeric;

#include<tuplelargetypes/numeric_extended.h>
#include<cutlery/value_arraylist.h>

#include<mpdecimal.h>

data_definitions_value_arraylist(digits_list, uint64_t)

/*
	materialized_numeric is a base 10^12 number
	very similar to numeric on the disk, it is to be used as a holder for writing and comparing decimal/numerics
	actual arithmetic computation must be done by putting them in acceptable structures for libmpdec
*/

// maximum number of base 10^12 digits that flow in and out of a materialized_numeric through the libmpdec conversions below
// it only limits (normalizes) the mpd_t conversions, it does not constrain the materialized_numeric container itself
// it must fit a uint32_t, since the digit counts everywhere in this header are uint32_t
#define MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT UINT32_C(1000000)
fail_build_on(MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT > UINT32_MAX);

// limits for the mpd_context_t that is sized just right for a materialized_numeric, all derived from the above macro and the on-disk format
// precision : every base 10^12 digit is worth 12 base 10 digits
#define MAX_MATERIALIZED_NUMERIC_MPD_PREC (((mpd_ssize_t)MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT) * 12)
// emax : highest base 10 adjusted exponent = most significant decimal position of the most significant base 10^12 digit at the highest int16_t exponent
#define MAX_MATERIALIZED_NUMERIC_MPD_EMAX ((((mpd_ssize_t)INT16_MAX) * 12) + 11)
// emin : lowest base 10 adjusted exponent = least significant decimal position of a lone base 10^12 digit at the lowest int16_t exponent
// (etiny = emin - prec + 1 then also covers the least significant digit of a full length materialized_numeric, as a subnormal)
#define MAX_MATERIALIZED_NUMERIC_MPD_EMIN (((mpd_ssize_t)INT16_MIN) * 12)

struct materialized_numeric
{
	numeric_sign_bits sign_bits;

	int16_t exponent;

	digits_list digits; // each uint64_t is a number between 0 including to 10^12 excluding
};

// generates a mpd_context_t sized just right for a materialized_numeric
// precision, emax and emin are set from the MAX_MATERIALIZED_NUMERIC_MPD_* macros above, rounding is half even and no traps are set (except allocation failure)
// always use this context (instead of mpd_maxcontext) when computing a mpd_t that is destined to become a materialized_numeric,
// so that libmpdec itself rounds/limits the result to what a materialized_numeric can represent
void get_mpd_context_for_materialized_numeric(mpd_context_t* ctx);

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

uint64_t get_nth_digit_from_materialized_numeric(const materialized_numeric* m, uint32_t position); // -> get digit at power of (10^12)^(exponent-position)
int set_nth_digit_in_materialized_numeric(materialized_numeric* m, uint64_t digit, uint32_t position);

int64_t maximum_power_of_digit_for_materialized_numeric(const materialized_numeric* m);
int64_t minimum_power_of_digit_for_materialized_numeric(const materialized_numeric* m);

uint64_t get_digit_from_materialized_numeric(const materialized_numeric* m, int64_t power);

int compare_materialized_numeric(const materialized_numeric* m1, const materialized_numeric* m2);

void negate_materialized_numeric(materialized_numeric* m);

void absolute_materialized_numeric(materialized_numeric* m);

// only the MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT most significant digits are exported, any excess least significant digits are truncated (toward zero) on the way out
mpd_t decimal_from_materialized_numeric(const materialized_numeric* m);

// exponent_too_big, will be set as an error if the final exponent can not fit 16 bits, so can not be represented by materialized_numeric or numeric on the disk
// d is expected to have been worked on under the context from get_mpd_context_for_materialized_numeric, so it already carries at most MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT base 10^12 digits
// if it was not, the excess least significant digits are truncated (toward zero) on the way in, keeping the result normalized
materialized_numeric decimal_to_materialized_numeric(const mpd_t* d, int* exponent_too_big);

void deinitialize_materialized_numeric(materialized_numeric* m);

void print_materialized_numeric(const materialized_numeric* m);

// is_zero OR if exponent >= (number of digits - 1)
int is_integral_materialized_numeric(const materialized_numeric* m);

#endif