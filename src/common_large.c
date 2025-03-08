#include<common_large.h>

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

int is_prefix_valid_and_not_NULL(const data_type_info* dti_p, positional_accessor pa);

uint64_t get_extension_head_page_id(const data_type_info* dti_p, positional_accessor pa);