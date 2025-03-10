#include<numeric_large.h>

#include<stdlib.h>

#include<relative_positional_accessor.h>

int is_numeric_short_type_info(const data_type_info* numeric_short_p)
{
	return strcmp(numeric_short_p->type_name, "numeric_short") == 0;
}

int is_numeric_large_type_info(const data_type_info* numeric_large_p)
{
	return strcmp(numeric_large_p->type_name, "numeric_large") == 0;
}

data_type_info* get_numeric_short_type_info(uint32_t max_size)
{
	data_type_info* dti_p = malloc(sizeof_tuple_data_type_info(3));
	if(dti_p == NULL)
		exit(-1);

	initialize_tuple_data_type_info(dti_p, "numeric_short", 1, max_size, 3);

	strcpy(dti_p->containees[0].field_name, "sign_bits");
	dti_p->containees[0].al.type_info = BIT_FIELD_NON_NULLABLE[3];

	strcpy(dti_p->containees[1].field_name, "exponent");
	dti_p->containees[1].al.type_info = INT_NON_NULLABLE[2];

	strcpy(dti_p->containees[1].field_name, "digits");
	{
		data_type_info* digits_dti_p = malloc(sizeof(data_type_info));
		if(digits_dti_p == NULL)
			exit(-1);
		(*digits_dti_p) = get_variable_element_count_array_type("numeric_digit", max_size, UINT_NON_NULLABLE[5]);
		dti_p->containees[1].al.type_info = digits_dti_p;
	}

	return dti_p;
}

data_type_info* get_numeric_large_type_info(const data_type_info* numeric_short_p, const page_access_specs* pas_p)
{
	data_type_info* dti_p = malloc(sizeof_tuple_data_type_info(2));
	if(dti_p == NULL)
		exit(-1);

	// the numeric_short controls the total size so we allow the blob_large to be atmost page_size bytes large
	initialize_tuple_data_type_info(dti_p, "numeric_large", 1, pas_p->page_size, 2);

	strcpy(dti_p->containees[0].field_name, "numeric_prefix");
	dti_p->containees[0].al.type_info = (data_type_info*)numeric_short_p;

	strcpy(dti_p->containees[1].field_name, "numeric_extension");
	dti_p->containees[1].al.type_info = (data_type_info*)(&(pas_p->page_id_type_info));

	return dti_p;
}

int extract_sign_bits_and_exponent_from_numeric(numeric_sign_bits* sign_bits, int16_t* exponent, const void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor)
{
	int is_large = 0;
	{
		const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
		if(is_numeric_large_type_info(dti_p))
			is_large = 1;
		else if(is_numeric_short_type_info(dti_p))
			is_large = 0;
		else
			return 0;
	}

	// why call this function if you do not need any thing
	if(sign_bits == NULL && exponent == NULL)
		return 1;

	relative_positional_accessor rpa;
	initialize_relative_positional_accessor(&rpa, &inline_accessor, 2);

	int result = 1;

	// extract sign_bits
	if(sign_bits != NULL && result == 1)
	{
		if(is_large)
			relative_positonal_accessor_set_from_relative(&rpa, STATIC_POSITION(0, 0));
		else
			relative_positonal_accessor_set_from_relative(&rpa, STATIC_POSITION(0));
		user_value sign_bits_uv;
		int valid = get_value_from_element_from_tuple(&sign_bits_uv, tpl_d, rpa.exact, tupl);
		if(valid && !is_user_value_NULL(&sign_bits_uv))
			(*sign_bits) = sign_bits_uv.bit_field_value;
		else
			result = 0;
	}

	// extract exponent
	if(exponent != NULL && result == 1)
	{
		if(is_large)
			relative_positonal_accessor_set_from_relative(&rpa, STATIC_POSITION(0, 1));
		else
			relative_positonal_accessor_set_from_relative(&rpa, STATIC_POSITION(1));
		user_value exponent_uv;
		int valid = get_value_from_element_from_tuple(&exponent_uv, tpl_d, rpa.exact, tupl);
		if(valid && !is_user_value_NULL(&exponent_uv))
			(*exponent) = exponent_uv.int_value;
		else
			result = 0;
	}

	deinitialize_relative_positional_accessor(&rpa);

	return result;
}

int compare_numeric_prefix_no_digits(numeric_sign_bits s1, int16_t e1, numeric_sign_bits s2, int16_t e2, int* digits_requirement)
{
	// start with comparing the sign bits
	int compare = compare_numbers(s1, s2);

	// if any of the number is a sign_bit only number, i.e +/-infinity or a zero, then comparison ends here
	if(IS_INFINITY_NUMERIC_SIGN_BIT(s1) || IS_ZERO_NUMERIC_SIGN_BIT(s1) || IS_INFINITY_NUMERIC_SIGN_BIT(s2) || IS_ZERO_NUMERIC_SIGN_BIT(s2))
	{
		(*digits_requirement) = 0;
		return compare;
	}

	// if the comparison result was already established then nothing else is needed
	if(compare != 0)
	{
		(*digits_requirement) = 0;
		return compare;
	}

	// now we have 2 positive numbers OR 2 negative numbers at hand
	compare = compare_numbers(e1, e2);
	if(IS_NEGATIVE_NUMERIC_SIGN_BIT(s1)) // if they are both negative numbers then the sign bit has to be inverted at every further comparison
		compare = -compare;

	if(compare != 0) // if the exponents were unequal then we are done
	{
		(*digits_requirement) = 0;
		return compare;
	}

	// if exponents were also equal then digits need to be compared
	(*digits_requirement) = 1;
	if(IS_NEGATIVE_NUMERIC_SIGN_BIT(s1)) // if they are both negative numbers then the sign bit has to be inverted at every further comparison
		(*digits_requirement) = -1;
	return 0;
}