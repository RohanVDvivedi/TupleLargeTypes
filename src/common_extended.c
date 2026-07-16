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

const char* get_extension_sub_type_for_extended_type(const data_type_info* dti_p, uint32_t* length)
{
	if(!is_extended_type_info(dti_p))
	{
		(*length) = 0;
		return NULL;
	}

	const char* res = NULL;

	const char* temp = dti_p->type_name;
	for(; (temp - dti_p->type_name) < 64 && (*temp) != '\0'; temp++)
		if((*temp) == '{')
		{
			res = (++temp);
			break;
		}

	(*length) = 0;
	if(res == NULL)
		return NULL;

	for(; (temp - dti_p->type_name) < 64 && (*temp) != '\0'; temp++, (*length)++)
		if((*temp) == '}')
			return res;

	(*length) = 0;
	return NULL;
}

static int does_any_have_suffix_in_type_name(const data_type_info* dti_p, const dstring* suffix)
{
	if(is_suffix_of_dstring(&get_dstring_pointing_to_cstring(dti_p->type_name), suffix))
		return 1;

	if(dti_p->type == ARRAY)
		return does_any_have_suffix_in_type_name(dti_p->containee, suffix);
	else if(dti_p->type == TUPLE)
	{
		int has_extended = 0;
		for(uint32_t i = 0; i < dti_p->element_count && !has_extended; i++)
			has_extended = does_any_have_suffix_in_type_name(dti_p->containees[i].al.type_info, suffix);
		return has_extended;
	}
	else
		return 0;
}

int has_extended_type_info(const data_type_info* dti_p, const char* for_extension_sub_type)
{
	char temp[128] = "";
	if(for_extension_sub_type != NULL)
		sprintf(temp, "_{%s}_" EXTENDED_TYPE_SUFFIX, for_extension_sub_type);
	else
		sprintf(temp, "_" EXTENDED_TYPE_SUFFIX);

	return does_any_have_suffix_in_type_name(dti_p, &get_dstring_pointing_to_cstring(temp));
}

int has_extended_type_info2(const tuple_def* tpl_d, positional_accessor pos, const char* for_extension_sub_type)
{
	char temp[128] = "";
	if(for_extension_sub_type != NULL)
		sprintf(temp, "_{%s}_" EXTENDED_TYPE_SUFFIX, for_extension_sub_type);
	else
		sprintf(temp, "_" EXTENDED_TYPE_SUFFIX);

	return does_any_have_suffix_in_type_name(get_type_info_for_element_from_tuple_def(tpl_d, pos), &get_dstring_pointing_to_cstring(temp));
}

int has_extended_type_info3(const tuple_def* tpl_d, uint32_t key_element_count, const positional_accessor* key_element_ids, const char* for_extension_sub_type)
{
	char temp[128] = "";
	if(for_extension_sub_type != NULL)
		sprintf(temp, "_{%s}_" EXTENDED_TYPE_SUFFIX, for_extension_sub_type);
	else
		sprintf(temp, "_" EXTENDED_TYPE_SUFFIX);

	for(uint32_t i = 0; i < key_element_count; i++)
		if(does_any_have_suffix_in_type_name(get_type_info_for_element_from_tuple_def(tpl_d, key_element_ids[i]), &get_dstring_pointing_to_cstring(temp)))
			return 1;

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
		const data_type_info* extension_head_dti;
		if(get_nested_containee_from_datum(&extension_head, &extension_head_dti, uval, dti, EXTENSION_HEAD_POS_ACC) && !is_datum_NULL(&extension_head))
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

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &pos, 1);
	relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENSION_HEAD_POS_ACC);

	set_element_in_tuple(tpl_d, child_relative_accessor.exact, tupl, &((datum){.tuple_value = chunk_pointer_tuple}), 0);

	deinitialize_relative_positional_accessor(&child_relative_accessor);
}