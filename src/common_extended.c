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

#include<tuplelargetypes/numeric_extended.h>

uint64_t get_extension_head_page_id_for_extended_type(const datum* uval, const data_type_info* dti, const page_access_specs* pas_p)
{
	if(is_extended_type_info(dti) && !is_datum_NULL(uval)) // extract only when uval is not NULL && the type is the extended one
	{
		datum prefix;
		const data_type_info* prefix_dti;
		int valid = get_nested_containee_from_datum(&prefix, &prefix_dti, uval, dti, EXTENDED_PREFIX_POS_ACC);
		if(!valid || is_datum_NULL(&prefix))
			return pas_p->NULL_PAGE_ID;

		if(is_numeric_extended_type_info(dti))
		{
			datum prefix_digits;
			const data_type_info* prefix_digits_dti;
			int valid = get_nested_containee_from_datum(&prefix_digits, &prefix_digits_dti, uval, dti, GET_NUMERIC_DIGITS_POS_ACC(is_extended_type_info(dti)));
			if(!valid || is_datum_NULL(&prefix_digits))
				return pas_p->NULL_PAGE_ID;
		}

		datum extension_head_page_id;
		const data_type_info* extension_head_page_id_dti;
		valid = get_nested_containee_from_datum(&extension_head_page_id, &extension_head_page_id_dti, uval, dti, EXTENDED_HEAD_PAGE_ID_POS_ACC);
		if(valid && (!is_datum_NULL(&extension_head_page_id)))
			return extension_head_page_id.uint_value;
	}

	return pas_p->NULL_PAGE_ID;
}