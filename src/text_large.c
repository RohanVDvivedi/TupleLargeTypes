#include<text_large.h>

#include<stdlib.h>

int is_text_short_type_info(const data_type_info* text_short_p)
{
	return strcmp(text_short_p->type_name, "text_short") == 0;
}

int is_text_large_type_info(const data_type_info* text_large_p)
{
	return strcmp(text_large_p->type_name, "text_large") == 0;
}

data_type_info* get_text_short_type_info(uint32_t max_size)
{
	data_type_info* dti_p = malloc(sizeof(data_type_info));
	if(dti_p == NULL)
		exit(-1);

	(*dti_p) = get_variable_length_string_type("text_short", max_size);

	return dti_p;
}

data_type_info* get_text_large_type_info(const data_type_info* text_short_p, const page_access_specs* pas_p)
{
	data_type_info* dti_p = malloc(sizeof_tuple_data_type_info(2));
	if(dti_p == NULL)
		exit(-1);

	initialize_tuple_data_type_info(dti_p, "text_large", 1, pas_p->page_size, 2);

	strcpy(dti_p->containees[0].field_name, "text_prefix");
	dti_p->containees[0].al.type_info = (data_type_info*)text_short_p;

	strcpy(dti_p->containees[1].field_name, "text_extension");
	dti_p->containees[1].al.type_info = (data_type_info*)(&(pas_p->page_id_type_info));

	return dti_p;
}