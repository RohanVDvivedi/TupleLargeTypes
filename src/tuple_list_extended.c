#include<tuple_list_extended.h>

#include<stdlib.h>

int is_tuple_list_extended_type_info(const data_type_info* dti_p)
{
	return strcmp(dti_p->type_name, "tuple_list_extended") == 0;
}

data_type_info* get_tuple_list_extended_type_info(uint32_t max_size, uint32_t inline_size, const page_access_specs* pas_p)
{
	data_type_info* dti_p = malloc(sizeof_tuple_data_type_info(2));
	if(dti_p == NULL)
		exit(-1);

	// the blob_inline controls the total size so we allow the blob_extended to be atmost page_size bytes large
	initialize_tuple_data_type_info(dti_p, "tuple_list_extended", 1, max_size, 2);

	strcpy(dti_p->containees[0].field_name, "tuple_list_prefix");
	{
		data_type_info* tuple_list_inline_p = malloc(sizeof(data_type_info));
		if(tuple_list_inline_p == NULL)
			exit(-1);

		(*tuple_list_inline_p) = get_variable_length_blob_type("tuple_list_inline", inline_size);

		dti_p->containees[0].al.type_info = (data_type_info*)tuple_list_inline_p;
	}

	strcpy(dti_p->containees[1].field_name, "tuple_list_extension");
	dti_p->containees[1].al.type_info = (data_type_info*)(&(pas_p->page_id_type_info));

	return dti_p;
}