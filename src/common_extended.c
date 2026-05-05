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

data_type_info* get_extendion_head_type_info(const page_access_specs* pas_p)
{
	data_type_info* dti_p = malloc(sizeof_tuple_data_type_info(2));
	if(dti_p == NULL)
		exit(-1);

	// the text_inline controls the total size so we allow the text_extended to be atmost page_size bytes large
	initialize_tuple_data_type_info(dti_p, "extension_head", 0, 0, 2);

	strcpy(dti_p->containees[0].field_name, "extension_head_page_id");
	dti_p->containees[0].al.type_info = (data_type_info*)(&(pas_p->page_id_type_info));

	strcpy(dti_p->containees[1].field_name, "extension_head_tuple_index");
	dti_p->containees[1].al.type_info = (data_type_info*)(&(pas_p->tuple_index_type_info));

	return dti_p;
}

#include<tuplelargetypes/numeric_extended.h>

chunk_ptr get_extension_head_for_extended_type(const datum* uval, const data_type_info* dti, const page_access_specs* pas_p)
{
	if(is_extended_type_info(dti) && !is_datum_NULL(uval)) // extract only when uval is not NULL && the type is the extended one
	{
		datum prefix;
		const data_type_info* prefix_dti;
		int valid = get_nested_containee_from_datum(&prefix, &prefix_dti, uval, dti, EXTENDED_PREFIX_POS_ACC);
		if(!valid || is_datum_NULL(&prefix))
			return (chunk_ptr){pas_p->NULL_PAGE_ID};

		if(is_numeric_extended_type_info(dti))
		{
			datum prefix_digits;
			const data_type_info* prefix_digits_dti;
			int valid = get_nested_containee_from_datum(&prefix_digits, &prefix_digits_dti, uval, dti, GET_NUMERIC_DIGITS_POS_ACC(is_extended_type_info(dti)));
			if(!valid || is_datum_NULL(&prefix_digits))
				return (chunk_ptr){pas_p->NULL_PAGE_ID};
		}

		datum uval_head_page_id;
		datum uval_head_tuple_index;
		const data_type_info* dti;
		if(get_nested_containee_from_datum(&uval_head_page_id, &dti, uval, dti, EXTENSION_HEAD_PAGE_ID_POS_ACC)
			&& get_nested_containee_from_datum(&uval_head_tuple_index, &dti, uval, dti, EXTENSION_HEAD_TUPLE_INDEX_POS_ACC))
			return (chunk_ptr){uval_head_page_id.uint_value, uval_head_tuple_index.uint_value};
	}

	return (chunk_ptr){pas_p->NULL_PAGE_ID};
}

void initialize_extension_head(void* tupl, const tuple_def* tpl_d, positional_accessor pos, const page_access_specs* pas_p)
{
	// make sure the type in context is an extended type
	{
		const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, pos);
		if(dti_p == NULL)
			return ;
		if(!is_extended_type_info(dti_p))
			return ;
	}

	// create a relative position accessor
	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &pos, 2);

	relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENSION_HEAD_PAGE_ID_POS_ACC);
	set_element_in_tuple(tpl_d, child_relative_accessor.exact, tupl, &((datum){.uint_value = pas_p->NULL_PAGE_ID}), UINT32_MAX);

	relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENSION_HEAD_TUPLE_INDEX_POS_ACC);
	set_element_in_tuple(tpl_d, child_relative_accessor.exact, tupl, &((datum){.uint_value = 0}), UINT32_MAX);

	deinitialize_relative_positional_accessor(&child_relative_accessor);
}