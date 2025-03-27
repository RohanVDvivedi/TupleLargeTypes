#ifndef NUMERIC_EXTENDED_H
#define NUMERIC_EXTENDED_H

#include<tuple.h>
#include<data_type_info.h>
#include<page_access_specification.h>

#define BYTES_PER_NUMERIC_DIGIT 5
fail_build_on(BYTES_PER_NUMERIC_DIGIT > 8);

// below two functions only check the type_name of the dti passed
int is_numeric_inline_type_info(const data_type_info* dti_p);
int is_numeric_extended_type_info(const data_type_info* dti_p);

// returns a new type info pointing to inline type that is atmost max_size bytes big
// max_size = 2 * ceil(log(page_size) base 256) + 1 (bytes for sign bit field) + 2 (bytes for exponent) + BYTES_PER_NUMERIC_DIGIT * ceil(digits / 12)
// for 8 KB page and storing 36 digits, max_size = 2 * 2 + 1 + 2 + 5 * 3 = 22 bytes of max size
data_type_info* get_numeric_inline_type_info(uint32_t max_size);

// returns a new type info pointing to a tuple, of 2 elements the numeric_inline_p, and a page_id type from pas_p
data_type_info* get_numeric_extended_type_info(uint32_t max_size, const data_type_info* numeric_inline_p, const page_access_specs* pas_p);

// for reason to ease comparison of the inline numeric directly using the TupleStore functions
// the information about the numeric being infinity has been coded directly into the sign bits and that too in order that results in correct default comparison
typedef enum numeric_sign_bits numeric_sign_bits;
enum numeric_sign_bits
{
	NEGATIVE_INFINITY_NUMERIC = 0,
	NEGATIVE_NUMERIC = 1,
	ZERO_NUMERIC = 2,
	POSITIVE_NUMERIC = 3,
	POSITIVE_INFINITY_NUMERIC = 4,
};
#define IS_INFINITY_NUMERIC_SIGN_BIT(s) (((s) == NEGATIVE_INFINITY_NUMERIC) || ((s) == POSITIVE_INFINITY_NUMERIC))
#define IS_NEGATIVE_NUMERIC_SIGN_BIT(s) (((s) <= NEGATIVE_NUMERIC))
#define IS_POSITIVE_NUMERIC_SIGN_BIT(s) (((s) >= POSITIVE_NUMERIC))
#define IS_ZERO_NUMERIC_SIGN_BIT(s)     (((s) == ZERO_NUMERIC))
extern char const * numeric_sign_bits_str[5];

// pass in/out parameter sign_bits and exponent based on what ever you need
int extract_sign_bits_and_exponent_from_numeric(numeric_sign_bits* sign_bits, int16_t* exponent, const void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor);

// returns 0, if any of the set calls either for the sign_bits or the exponent fails
int set_sign_bits_and_exponent_for_numeric(numeric_sign_bits sign_bits, int16_t exponent, void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor);

// below functions can be used to compare numeric prefix using just sign_bits (s1 and s2) and exponent (e1 and e2), without consulting the digits
// a flag digits_requirement will be set if the further comparison of digits will be necessary
// NOTE: this functions assumes that the numeric is stored in correct scientific notation of base 10^12, as explained in the large comment at the end of this file
// digits_requirement -> 0, nothing needs to be done, return value is the result of comparison
//                    -> else, multiply the sign of this variable in what we get from comparing the digits (this will be 1, if they are both positive, and, -1, if they are both negative)
int compare_numeric_prefix_no_digits(numeric_sign_bits s1, int16_t e1, numeric_sign_bits s2, int16_t e2, int* digits_requirement);

#include<common_extended.h>

#include<digit_read_iterator.h>
#include<digit_write_iterator.h>

#include<binary_hasher.h>

#endif

/*
	INLINE NUMERIC TYPE
	numeric type here is represented as a base 10^12 number, with each digit being a 5 byte unsigned number between [0, 10^12), in-memory representation of each digit will be a uint64_t
	each inline numeric type is composed of
		* 3 sign bits ==> 0 -> -infinity, 1 -> negative number, 2 -> zero (there are not +/- zeros just a zero), 3 -> positive number and 4 -> +infinity
		* 2 byte signed exponent
		* some N number of digits as an array of 5 byte unsigned integers

	Explanation of choosing a 5 byte integer to store 12 decimal digits of information comes from the following
		* storing mantissa is a big problem, we need to store digit groups as unsigned integers
		* an X byte big unsigned integer can store Y digits, then Y = log(256 ^ X) / log(10) = X * 8 * log(2) / log(10)
		* the table goes as follows
			X |    Y   ->  ceil(Y) |  Y/X
			1 |  2.408 ->  2       |  2
			2 |  4.816 ->  4       |  2
			3 |  7.224 ->  7       |  2.333
			4 |  9.632 ->  9       |  2.25
			5 | 12.041 -> 12       |  2.4
			6 | 14.449 -> 14       |  2.333
			7 | 16.857 -> 16       |  2.285
			8 | 19.265 -> 19       |  2.375
		* if you analyze carefully the most value of digits per bytes is the most efficient way to go, which is using 5 byte unsigned integer to store a group of 12 decimal digits
		* first task is to build a function to set one of the 12 digits in a uint64_t
		* now since we are storing digits in base 10^12 in a 5 byte integer, why not store exponent represented as a (10^12) ^ exponent
		* now everything becomes radix 10^12 instead of radix of 10, and with 3 sign bits, 16 bit exponent and finally an array of 5 byte integers, each storing precisely 12 digits worth of information as a 10^12 radix digit

	EXTENDED NUMERIC TYPE
	will just be a tuple of a inline numeric type and a page_id to point to a worm
	any and all following digit that could not be accomodated in the prefix will be pushed into the worm in 5 byte chunks one after the another
*/