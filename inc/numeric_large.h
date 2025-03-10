#ifndef NUMERIC_LARGE_H
#define NUMERIC_LARGE_H

#include<tuple.h>
#include<data_type_info.h>
#include<page_access_specification.h>

// below two functions only check the type_name of the dti passed
int is_numeric_short_type_info(const data_type_info* numeric_short_p);
int is_numeric_large_type_info(const data_type_info* numeric_large_p);

// returns a new type info pointing to short type that is atmost max_size bytes big
// max_size = 2 * ceil(log(page_size) base 256) + 1 (bytes for bit field) + 2 (bytes for exponent) + 5 * ceil(digits / 12)
// for 8 KB page and storing 36 digits, max_size = 2 * 2 + 1 + 2 + 5 * 3 = 22 bytes of max size
data_type_info* get_numeric_short_type_info(uint32_t max_size);

// returns a new type info pointing to a tuple, of 2 elements the numeric_short_p, and a page_id type from pas_p
data_type_info* get_numeric_large_type_info(const data_type_info* numeric_short_p, const page_access_specs* pas_p);

// for reason to easy comparison of the short numeric directly using the TupleStore functions
// the information about the numeric being infinity has been coded directly into the sign bits and that too in order that results in correct default comparison
typedef enum numeric_sign_bits numeric_sign_bits;
enum numeric_sign_bits
{
	NEGATIVE_INFINITY_NUMERIC = 0,
	NEGATIVE_NUMERIC = 1,
	POSITIVE_NUMERIC = 2,
	POSITIVE_INFINITY_NUMERIC = 3,
};

// pass in/out parameter sign_bits and exponent based on what ever you need
int extract_sign_bits_and_exponent_from_numeric(numeric_sign_bits* sign_bits, int16_t* exponent, const void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor);

#include<common_large.h>

#endif

/*
	SHORT NUMERIC TYPE
	numeric type here is represented as a base 10^12 number, with each digit being a 5 byte unsigned number between [0, 10^12), in-memory representation of each digit will be a uint64_t
	each shot numeric type is composed of
		* 2 sign bits ==> 00 -> -infinity, 01 -> negative number, 10 -> positive number and 11 -> +infinity
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
		* now everything becomes radix 10^12 instead of radix of 10, and with 2 sign bits, 16 bit exponent and finally an array of 5 byte integers, each storing precisely 12 digits worth of information as a 10^12 radix digit

	LARGE NUMERIC TYPE
	will just be a tuple of a short numeric type and a page_id to point to a worm
	any and all following digit that could not be accomodated in the prefix will be pushed into the worm in 5 byte chunks one after the another
*/