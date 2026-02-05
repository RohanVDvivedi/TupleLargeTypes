#include<tuplelargetypes/common_extended.h>

#include<stdlib.h>

#include<tuplelargetypes/relative_positional_accessor.h>

int is_inline_type_info(const data_type_info* dti_p)
{
	return !is_extended_type_info(dti_p);
}

int is_extended_type_info(const data_type_info* dti_p)
{
	int type_name_length = strnlen(dti_p->type_name, sizeof(dti_p->type_name));
	int _extended_length = strlen("_extended");
	if(type_name_length < _extended_length)
		return 0;
	return strcmp(dti_p->type_name + type_name_length - _extended_length, "_extended") == 0;
}

uint64_t get_extension_head_page_id_for_extended_type(const void* tupl, const tuple_def* tpl_d, positional_accessor pa, const page_access_specs* pas_p)
{
	// ensure that it is a large type_info
	int is_numeric = 0;
	{
		const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, pa);
		if(!is_extended_type_info(dti_p))
			return pas_p->NULL_PAGE_ID;
		is_numeric = strncmp(dti_p->type_name, "numeric", sizeof("numeric")-1) == 0;
	}

	relative_positional_accessor rpa;
	initialize_relative_positional_accessor(&rpa, &pa, 2);

	// before a worm is created and appended to, the prefix has to be non null and valid, (it may not be full, but it must have something)
	{
		if(is_numeric)
			relative_positonal_accessor_set_from_relative(&rpa, STATIC_POSITION(0, 2));
		else
			relative_positonal_accessor_set_from_relative(&rpa, STATIC_POSITION(0));
		datum prefix;
		int valid = get_value_from_element_from_tuple(&prefix, tpl_d, rpa.exact, tupl);
		if((!valid) || is_datum_NULL(&prefix))
		{
			deinitialize_relative_positional_accessor(&rpa);
			return pas_p->NULL_PAGE_ID;
		}
	}

	uint64_t extension_head_page_id = pas_p->NULL_PAGE_ID;
	{
		relative_positonal_accessor_set_from_relative(&rpa, STATIC_POSITION(1));
		datum extension_head_page;
		int valid = get_value_from_element_from_tuple(&extension_head_page, tpl_d, rpa.exact, tupl);
		if(valid && (!is_datum_NULL(&extension_head_page)))
			extension_head_page_id = extension_head_page.uint_value;
	}

	deinitialize_relative_positional_accessor(&rpa);
	return extension_head_page_id;
}