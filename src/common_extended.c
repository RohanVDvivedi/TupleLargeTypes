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

uint64_t get_or_create_extension_worm(void* tupl, const tuple_def* tpl_d, positional_accessor pos, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p, const void* transaction_id, int* abort_error)
{
	// make sure the type in context is an extended type
	{
		const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, pos);
		if(dti_p == NULL)
			return pam_p->pas.NULL_PAGE_ID;
		if(!is_extended_type_info(dti_p))
			return pam_p->pas.NULL_PAGE_ID;
	}

	// initialize it to NULL
	uint64_t extension_head_page_id = pam_p->pas.NULL_PAGE_ID;

	// create a relative position accessor
	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &pos, 1);
	relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENDED_HEAD_PAGE_ID_POS_ACC);

	// fetch the extension_head_page_id
	{
		datum extension_head_page_id_uval;
		if(!get_value_from_element_from_tuple(&extension_head_page_id_uval, tpl_d, child_relative_accessor.exact, tupl)) // if we can not extraxt one, fail this call
		{
			deinitialize_relative_positional_accessor(&child_relative_accessor);
			return pam_p->pas.NULL_PAGE_ID;
		}
		if(is_datum_NULL(&extension_head_page_id_uval))
			extension_head_page_id = pam_p->pas.NULL_PAGE_ID;
		else
			extension_head_page_id = extension_head_page_id_uval.uint_value;
	}

	// if extension_head_page_id is NULL_PAGE_ID, then create a new worm and set it in the attribute beside prefix
	if(extension_head_page_id == pam_p->pas.NULL_PAGE_ID)
	{
		extension_head_page_id = get_new_worm(1, pam_p->pas.NULL_PAGE_ID, wtd_p, pam_p, pmm_p, transaction_id, abort_error);
		if(*abort_error)
		{
			deinitialize_relative_positional_accessor(&child_relative_accessor);
			return pam_p->pas.NULL_PAGE_ID;
		}
		set_element_in_tuple(tpl_d, child_relative_accessor.exact, tupl, &((datum){.uint_value = extension_head_page_id}), UINT32_MAX);
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);
	return extension_head_page_id;
}

void delete_all_extension_worms(const datum* uval, const data_type_info* dti, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p, const void* transaction_id, int* abort_error)
{
	// return directly if the uval is NULL
	if(is_datum_NULL(uval))
		return;

	// if not a container, return directly
	// skip a BINARY/STRING also here
	if(dti->type != TUPLE && dti->type != ARRAY)
		return;

	// if already an extended type, delete an extension if it has any
	if(is_extended_type_info(dti))
	{
		uint64_t extension_head_page_id = get_extension_head_page_id_for_extended_type(uval, dti, &(pam_p->pas));
		if(extension_head_page_id != pam_p->pas.NULL_PAGE_ID)
		{
			uint64_t dependent_root_page_id;
			int vaccum_needed;
			decrement_reference_counter_for_worm(extension_head_page_id, &dependent_root_page_id, &vaccum_needed, wtd_p, pam_p, pmm_p, transaction_id, abort_error);
			if(*abort_error)
				return;
		}
		return;
	}

	uint32_t element_count = get_element_count_for_datum(uval, dti);
	for(uint32_t i = 0; i < element_count; i++)
	{
		datum uval_c;
		const data_type_info* dti_c;
		int valid = get_containee_from_datum(&uval_c, &dti_c, uval, dti, i);
		if(valid)
		{
			delete_all_extension_worms(&uval_c, dti_c, wtd_p, pam_p, pmm_p, transaction_id, abort_error);
			if(*abort_error)
				return;
		}
	}
}