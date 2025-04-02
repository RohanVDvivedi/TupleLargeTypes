#include<tuplelargetypes/materialized_numeric.h>

declarations_value_arraylist(digits_list, uint64_t, static inline)
#define EXPANSION_FACTOR 1.5
function_definitions_value_arraylist(digits_list, uint64_t, static inline)

int initialize_materialized_numeric(materialized_numeric* m, uint32_t digits_capacity);
int initialize_static_materialized_numeric(materialized_numeric* m, numeric_sign_bits sign_bits, int16_t exponent, uint64_t* digits_array, uint32_t digits_count);

void get_sign_bits_and_exponent_for_materialized_numeric(const materialized_numeric* m, numeric_sign_bits* sign_bits, int16_t* exponent);
void set_sign_bits_and_exponent_for_materialized_numeric(materialized_numeric* m, numeric_sign_bits sign_bits, int16_t exponent);

uint32_t get_digits_count_for_materialized_numeric(const materialized_numeric* m);

// below two functions fail if you try to append to the -inf, 0 or +inf numerics
int push_msd_in_materialized_numeric(materialized_numeric* m, uint64_t digit);
int push_lsd_in_materialized_numeric(materialized_numeric* m, uint64_t digit);

uint64_t get_nth_digit_from_materialized_numeric(const materialized_numeric* m, uint32_t position); // -> get digit at power of (10^12)^(-position)
int set_nth_digit_in_materialized_numeric(materialized_numeric* m, uint64_t digit, uint32_t position);

int compare_materialized_numeric(const materialized_numeric* m1, const materialized_numeric* m2);

void deinitialize__materialized_numeric(materialized_numeric* m);