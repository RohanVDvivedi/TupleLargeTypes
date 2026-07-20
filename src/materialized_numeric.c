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
	if(IS_INFINITY_NUMERIC_SIGN_BIT(sign_bits) || IS_ZERO_NUMERIC_SIGN_BIT(sign_bits) || IS_NAN_NUMERIC_SIGN_BIT(sign_bits))
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

int64_t maximum_power_of_digit_for_materialized_numeric(const materialized_numeric* m)
{
	return m->exponent;
}

int64_t minimum_power_of_digit_for_materialized_numeric(const materialized_numeric* m)
{
	if(get_element_count_digits_list(&(m->digits)) == 0)
		return m->exponent;

	return ((int64_t)(m->exponent)) - get_element_count_digits_list(&(m->digits)) + 1;
}

uint64_t get_digit_from_materialized_numeric(const materialized_numeric* m, int64_t power)
{
	if(power < minimum_power_of_digit_for_materialized_numeric(m) || maximum_power_of_digit_for_materialized_numeric(m) < power)
		return 0;

	return get_nth_digit_from_materialized_numeric(m, m->exponent - power);
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

	if(cmp == 0)
	{
		if(digits_count1 != digits_count2)
		{
			if(digits_count1 > digits_count2)
				cmp = 1;
			else
				cmp = -1;
		}
	}

	return cmp * digits_requirement;
}

void negate_materialized_numeric(materialized_numeric* m)
{
	m->sign_bits = negate_numeric_sign_bits(m->sign_bits);
}

void absolute_materialized_numeric(materialized_numeric* m)
{
	if(IS_NEGATIVE_SIGN_BIT(m->sign_bits))
		negate_materialized_numeric(m);
}

void get_mpd_context_for_materialized_numeric(mpd_context_t* ctx)
{
	// start from the max context : half even rounding, all statuses recorded, no traps (except allocation failure)
	mpd_maxcontext(ctx);

	// then shrink it to be just right for a materialized_numeric
	// these can not fail, since the values below are strictly within the mpd_maxcontext limits, but we still check
	if(!mpd_qsetprec(ctx, MAX_MATERIALIZED_NUMERIC_MPD_PREC))
		exit(-1);
	if(!mpd_qsetemax(ctx, MAX_MATERIALIZED_NUMERIC_MPD_EMAX))
		exit(-1);
	if(!mpd_qsetemin(ctx, MAX_MATERIALIZED_NUMERIC_MPD_EMIN))
		exit(-1);
}

mpd_t decimal_from_materialized_numeric(const materialized_numeric* m)
{
	// the import below is done under the max context and NOT under get_mpd_context_for_materialized_numeric :
	// mpd_qimport_u32 finalizes the coefficient at exponent 0, and for a large digit count that intermediate
	// (not the final value) has an adjusted exponent way above the just-right emax, and would falsely overflow to infinity.
	// the just-right context is applied only at the very end, once the real exponent has been set.
	mpd_context_t maxctx;
	mpd_maxcontext(&maxctx);

	// res is returned by value : its struct is treated as static (it is copied out to the caller),
	// while its coefficient buffer (res.data) is heap allocated.
	// THE CALLER MUST RELEASE THE RETURNED VALUE WITH mpd_del() -> that frees res.data, not the struct.
	mpd_t res;
	res.flags = MPD_STATIC; // static struct (copied out to the caller) with a heap allocated coefficient buffer
	res.exp = 0;
	res.digits = 0;
	res.len = 0;
	res.alloc = MPD_MINALLOC;
	res.data = mpd_alloc(MPD_MINALLOC, sizeof(mpd_uint_t)); // allocate the coefficient buffer on the heap
	if(res.data == NULL)
		exit(-1);

	uint8_t sign = 0;
	int needs_digits = 0;

	switch(m->sign_bits)
	{
		case NEGATIVE_INFINITY_NUMERIC :
		{
			mpd_setspecial(&res, MPD_NEG, MPD_INF);
			break;
		}
		case NEGATIVE_NUMERIC :
		{
			sign = MPD_NEG;
			needs_digits = 1;
			break;
		}
		case ZERO_NUMERIC :
		{
			mpd_zerocoeff(&res);
			mpd_set_flags(&res, MPD_POS);
			break;
		}
		case POSITIVE_NUMERIC :
		{
			sign = MPD_POS;
			needs_digits = 1;
			break;
		}
		case POSITIVE_INFINITY_NUMERIC :
		{
			mpd_setspecial(&res, MPD_POS, MPD_INF);
			break;
		}
		case NAN_NUMERIC :
		{
			mpd_setspecial(&res, MPD_POS, MPD_NAN);
			break;
		}
	}

	// a finite non zero number with actual digits : import its coefficient
	if(needs_digits && get_digits_count_for_materialized_numeric(m) > 0)
	{
		uint32_t digits_count = get_digits_count_for_materialized_numeric(m);

		// limit the outgoing digits : only the MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT most significant digits go out,
		// the excess least significant ones are dropped (truncation toward zero), by skipping that many digits from the back
		// and raising the power of the least significant exported digit by the same amount
		uint32_t skipped_lsd_count = 0;
		if(digits_count > MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT)
		{
			skipped_lsd_count = digits_count - MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT;
			digits_count = MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT;
		}

		// each base 10^12 digit becomes 2 base 10^6 words, in little endian order for mpd_qimport_u32
		uint32_t* digits = malloc(sizeof(uint32_t) * digits_count * 2);
		if(digits == NULL)
			exit(-1);
		for(uint32_t i = 0; i < digits_count; i++)
		{
			uint64_t d = *get_from_back_of_digits_list(&(m->digits), i + skipped_lsd_count); // i-th from the back == i-th least significant exported digit
			digits[2*i] = d % 1000000ULL;
			digits[2*i+1] = d / 1000000ULL;
		}

		uint32_t status = 0;
		mpd_qimport_u32(&res, digits, digits_count * 2, sign, 1000000ULL, &maxctx, &status);
		free(digits);
		if(status & MPD_Malloc_error)
			exit(-1);

		// mpd_qimport_u32 imports only the coefficient and resets the exponent to 0,
		// so the real base 10 exponent must be set AFTER the import.
		// the least significant exported digit sits at power (exponent - (digits_count - 1)) of 10^12, i.e. * 12 in base 10.
		// (skipped_lsd_count is already accounted for : digits_count here counts only the exported digits.)
		res.exp = (((int64_t)(m->exponent)) - ((int64_t)digits_count - 1)) * 12;
	}
	else if(needs_digits) // a positive/negative number carrying no digits is just a zero
	{
		mpd_zerocoeff(&res);
		mpd_set_flags(&res, MPD_POS);
	}

	// only now, with the real exponent in place, finalize under the just-right context :
	// any in-limit materialized_numeric fits its precision, emax and etiny exactly, so this is a pure validation and never rounds or clamps
	mpd_context_t ctx;
	get_mpd_context_for_materialized_numeric(&ctx);
	uint32_t status = 0;
	mpd_qfinalize(&res, &ctx, &status);
	return res;
}

materialized_numeric decimal_to_materialized_numeric(const mpd_t* d, int* exponent_too_big)
{
	// default to no error
	(*exponent_too_big) = 0;

	materialized_numeric res;
	if(!initialize_materialized_numeric(&res, 0))
		exit(-1);

	if(mpd_isnan(d))
	{
		res.sign_bits = NAN_NUMERIC;
		return res;
	}

	if(mpd_isinfinite(d))
	{
		res.sign_bits = mpd_isnegative(d) ? NEGATIVE_INFINITY_NUMERIC : POSITIVE_INFINITY_NUMERIC;
		return res;
	}

	if(mpd_iszero(d))
	{
		res.sign_bits = ZERO_NUMERIC;
		return res;
	}

	// finite non zero number
	numeric_sign_bits sign_bits = mpd_isnegative(d) ? NEGATIVE_NUMERIC : POSITIVE_NUMERIC;

	// value = coefficient * 10^exponent. base 10^12 digits must align to a 12 decimal boundary.
	// instead of realigning the packed digits ourselves, let libmpdec shift the coefficient left by
	// (exponent mod 12) decimal places -- a power of 10 smaller than 10^12 -- and subtract the same
	// amount from the exponent. this keeps the value unchanged and makes the exponent a multiple of 12,
	// so the exported coefficient packs straight into base 10^12 digits with no further arithmetic.
	int64_t exponent = d->exp;
	int64_t shift = ((exponent % 12) + 12) % 12;   // 0 .. 11, so 10^shift < 10^12
	int64_t lsd_power = (exponent - shift) / 12;   // exponent - shift is a multiple of 12 (explicit floor)

	uint32_t status = 0;
	mpd_t* scaled = mpd_qnew();
	if(scaled == NULL)
		exit(-1);
	if(!mpd_qshiftl(scaled, d, (mpd_ssize_t)shift, &status)) // coefficient *= 10^shift, heavy lifting by libmpdec
		exit(-1);
	scaled->exp = 0; // export only the (already scaled) coefficient, not the value

	uint32_t* words = NULL;
	size_t words_count = mpd_qexport_u32(&words, 0, 1000000ULL, scaled, &status);
	mpd_del(scaled);
	if(words_count == SIZE_MAX) // export failed
		exit(-1);

	// 2 base 10^6 words make 1 base 10^12 digit, little endian (a missing high word counts as 0).
	uint32_t digits_count = (words_count + 1) / 2;

	// limit the incoming digits : d is expected to have been worked on under get_mpd_context_for_materialized_numeric,
	// whose precision already bounds it to MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT base 10^12 digits, so this normally never triggers.
	// if d was built under a larger context anyway, the excess least significant digits are dropped (truncation toward zero),
	// raising the power of the new least significant digit accordingly.
	// the zero skip below then runs after this truncation, so any newly exposed least significant zero digits are also dropped (canonical form).
	uint32_t start = 0;
	if(digits_count > MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT)
	{
		uint32_t excess_digits_count = digits_count - MAX_MATERIALIZED_NUMERIC_DIGIT_COUNT;
		start += excess_digits_count;
		lsd_power += excess_digits_count;
	}

	// skip least significant zero digits : they only raise the power and must not be stored (canonical form).
	// the coefficient has no leading zeros, so the most significant digit is always non zero (no high skip needed).
	while(start < digits_count)
	{
		uint64_t lo = words[2*start];
		uint64_t hi = (2*start + 1 < words_count) ? words[2*start + 1] : 0;
		if(lo != 0 || hi != 0)
			break;
		start++;
		lsd_power++;
	}
	uint32_t final_digits_count = digits_count - start; // significant digits after the limit truncation and the zero skip

	// this is the exact exponent that will be stored in the materialized_numeric (power of the most
	// significant digit). the saturation is done on THIS final exponent, not on the raw base 10 mpd
	// exponent, because this is the value that must fit the 2 byte (int16_t) exponent field.
	int64_t final_exponent = lsd_power + ((int64_t)final_digits_count) - 1;
	if(final_exponent > INT16_MAX) // magnitude too large for the 2 byte exponent -> saturate to +/- infinity
	{
		(*exponent_too_big) = 1;
		res.sign_bits = (sign_bits == NEGATIVE_NUMERIC) ? NEGATIVE_INFINITY_NUMERIC : POSITIVE_INFINITY_NUMERIC;
		mpd_free(words);
		return res;
	}
	if(final_exponent < INT16_MIN) // magnitude too small for the 2 byte exponent -> underflows to zero
	{
		(*exponent_too_big) = 1;
		res.sign_bits = ZERO_NUMERIC;
		mpd_free(words);
		return res;
	}

	// combine each word pair into a base 10^12 digit and push it straight in, least significant first :
	// push_msd keeps prepending, so the digits end up most significant first as required.
	set_sign_bits_and_exponent_for_materialized_numeric(&res, sign_bits, (int16_t)final_exponent);
	for(uint32_t i = start; i < digits_count; i++)
	{
		uint64_t lo = words[2*i];
		uint64_t hi = (2*i + 1 < words_count) ? words[2*i + 1] : 0;
		push_msd_in_materialized_numeric(&res, lo + hi * 1000000ULL);
	}

	mpd_free(words);
	return res;
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
		case NAN_NUMERIC :
		{
			printf("NAN");
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

int is_integral_materialized_numeric(const materialized_numeric* m)
{
	return (m->sign_bits == ZERO_NUMERIC) || (((m->sign_bits == NEGATIVE_NUMERIC) || (m->sign_bits == POSITIVE_NUMERIC)) && ((m->exponent >= 0) && m->exponent >= (get_digits_count_for_materialized_numeric(m) - 1)));
}