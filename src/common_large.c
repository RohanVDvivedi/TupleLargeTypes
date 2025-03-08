#include<common_large.h>

#include<stdlib.h>

int is_short_type_info(const data_type_info* dti_p)
{
	int type_name_length = strnlen(dti_p->type_name, sizeof(dti_p->type_name));
	int _short_length = strlen("_short");
	if(type_name_length < _short_length)
		return 0;
	return strcmp(dti_p->type_name + type_name_length - _short_length, "_short") == 0;
}

int is_large_type_info(const data_type_info* dti_p)
{
	int type_name_length = strnlen(dti_p->type_name, sizeof(dti_p->type_name));
	int _large_length = strlen("_large");
	if(type_name_length < _large_length)
		return 0;
	return strcmp(dti_p->type_name + type_name_length - _large_length, "_large") == 0;
}

uint64_t get_extension_head_page_id_for_large_type(const void* tupl, const tuple_def* tpl_d, positional_accessor pa, const page_access_specs* pas_p)
{
	// ensure that it is a large type_info
	{
		const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, pa);
		if(!is_large_type_info(dti_p))
			return pas_p->NULL_PAGE_ID;
	}

	positional_accessor pa_temp = {.positions_length = 0, .positions = malloc(sizeof(uint32_t) * pa.positions_length)};
	if(pa_temp.positions == NULL)
		exit(-1);
	append_positions(&pa_temp, pa);

	// before a worm is created and appended to, the prefix has to be non null and valid, (it may not be full, but it must have something)
	{
		//pa_temp.positions_length = pa.positions_length;
		append_positions(&pa_temp, STATIC_POSITION(0));
		user_value prefix;
		int valid = get_value_from_element_from_tuple(&prefix, tpl_d, pa_temp, tupl);
		if((!valid) || is_user_value_NULL(&prefix))
		{
			free(pa_temp.positions);
			return pas_p->NULL_PAGE_ID;
		}
	}

	uint64_t extension_head_page_id = pas_p->NULL_PAGE_ID;
	{
		pa_temp.positions_length = pa.positions_length;
		append_positions(&pa_temp, STATIC_POSITION(1));
		user_value extension_head_page;
		int valid = get_value_from_element_from_tuple(&extension_head_page, tpl_d, pa_temp, tupl);
		if(valid && (!is_user_value_NULL(&extension_head_page)))
			extension_head_page_id = extension_head_page.uint_value;
	}

	free(pa_temp.positions);
	return extension_head_page_id;
}