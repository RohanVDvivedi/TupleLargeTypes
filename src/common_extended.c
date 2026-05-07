#include<tuplelargetypes/common_extended.h>

#include<stdlib.h>

#include<tuplelargetypes/relative_positional_accessor.h>

int is_inline_type_info(const data_type_info* dti_p)
{
	return !is_extended_type_info(dti_p);
}

int is_extended_type_info(const data_type_info* dti_p)
{
	return is_suffix_of_dstring(&get_dstring_pointing_to_cstring(dti_p->type_name), &get_dstring_pointing_to_literal_cstring(EXTENDED_TYPE_SUFFIX));
}

int has_extended_type_info(const data_type_info* dti_p)
{
	if(is_extended_type_info(dti_p))
		return 1;

	if(dti_p->type == ARRAY)
		return has_extended_type_info(dti_p->containee);
	else if(dti_p->type == TUPLE)
	{
		int has_extended = 0;
		for(uint32_t i = 0; i < dti_p->element_count && !has_extended; i++)
			has_extended = has_extended_type_info(dti_p->containees[i].al.type_info);
		return has_extended;
	}
	else
		return 0;
}

#include<tuplelargetypes/numeric_extended.h>

tuple_pointer get_extension_head_for_extended_type(const datum* uval, const data_type_info* dti, const page_access_specs* pas_p)
{
	if(is_extended_type_info(dti) && !is_datum_NULL(uval)) // extract only when uval is not NULL && the type is the extended one
	{
		datum prefix;
		const data_type_info* prefix_dti;
		int valid = get_nested_containee_from_datum(&prefix, &prefix_dti, uval, dti, EXTENDED_PREFIX_POS_ACC);
		if(!valid || is_datum_NULL(&prefix))
			return get_NULL_tuple_pointer(pas_p);

		if(is_numeric_extended_type_info(dti))
		{
			datum prefix_digits;
			const data_type_info* prefix_digits_dti;
			int valid = get_nested_containee_from_datum(&prefix_digits, &prefix_digits_dti, uval, dti, GET_NUMERIC_DIGITS_POS_ACC(is_extended_type_info(dti)));
			if(!valid || is_datum_NULL(&prefix_digits))
				return get_NULL_tuple_pointer(pas_p);
		}

		datum extension_head;
		const data_type_info* dti;
		if(get_nested_containee_from_datum(&extension_head, &dti, uval, dti, EXTENSION_HEAD_POS_ACC) && is_datum_NULL(&extension_head))
			return get_tuple_pointer(extension_head.tuple_value, pas_p);
	}

	return get_NULL_tuple_pointer(pas_p);
}

void set_extension_head_for_extended_type(void* tupl, const tuple_def* tpl_d, positional_accessor pos, const page_access_specs* pas_p, tuple_pointer cptr)
{
	// make sure the type in context is an extended type
	{
		const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, pos);
		if(dti_p == NULL)
			return ;
		if(!is_extended_type_info(dti_p))
			return ;
	}

	char chunk_pointer_tuple[sizeof(tuple_pointer)];
	set_tuple_pointer(chunk_pointer_tuple, cptr, pas_p);

	set_element_in_tuple(tpl_d, EXTENSION_HEAD_POS_ACC, tupl, &((datum){.tuple_value = chunk_pointer_tuple}), 0);
}