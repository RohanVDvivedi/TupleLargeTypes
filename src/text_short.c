#include<text_short.h>

#include<stdlib.h>

data_type_info* get_text_short_type_info(uint32_t max_size)
{
	data_type_info* dti_p = malloc(sizeof(data_type_info));
	if(dti_p == NULL)
		exit(-1);

	(*dti_p) = get_variable_length_string_type("text_short", max_size);

	return dti_p;
}