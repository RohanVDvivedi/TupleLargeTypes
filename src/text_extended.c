#include<tuplelargetypes/text_extended.h>

#include<stdlib.h>

int is_text_inline_type_info(const data_type_info* dti_p)
{
	return strcmp(dti_p->type_name, TEXT_TYPE_PREFIX INLINE_TYPE_SUFFIX) == 0;
}

int is_text_extended_type_info(const data_type_info* dti_p)
{
	return strcmp(dti_p->type_name, TEXT_TYPE_PREFIX EXTENDED_TYPE_SUFFIX) == 0;
}

data_type_info* get_text_inline_type_info(int is_fixed, uint32_t max_size)
{
	data_type_info* dti_p = malloc(sizeof(data_type_info));
	if(dti_p == NULL)
		exit(-1);

	if(is_fixed)
		(*dti_p) = get_fixed_length_string_type(TEXT_TYPE_PREFIX INLINE_TYPE_SUFFIX, max_size, 1 /*is_nullable*/);
	else
		(*dti_p) = get_variable_length_string_type(TEXT_TYPE_PREFIX INLINE_TYPE_SUFFIX, max_size);

	return dti_p;
}

data_type_info* get_text_extended_type_info(uint32_t max_size, const data_type_info* text_inline_p, const page_access_specs* pas_p)
{
	data_type_info* dti_p = malloc(sizeof_tuple_data_type_info(2));
	if(dti_p == NULL)
		exit(-1);

	// the text_inline controls the total size so we allow the text_extended to be atmost page_size bytes large
	initialize_tuple_data_type_info(dti_p, TEXT_TYPE_PREFIX EXTENDED_TYPE_SUFFIX, 1, max_size, 2);

	strcpy(dti_p->containees[0].field_name, "text_prefix");
	dti_p->containees[0].al.type_info = (data_type_info*)text_inline_p;

	strcpy(dti_p->containees[1].field_name, "text_extension");
	dti_p->containees[1].al.type_info = (data_type_info*)(&(pas_p->page_id_type_info));

	return dti_p;
}