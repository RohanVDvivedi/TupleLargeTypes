#include<blob_large.h>

#include<stdlib.h>

data_type_info* get_blob_short_type_info(uint32_t max_size)
{
	data_type_info* dti_p = malloc(sizeof(data_type_info));
	if(dti_p == NULL)
		exit(-1);

	(*dti_p) = get_variable_length_blob_type("blob_short", max_size);

	return dti_p;
}

data_type_info* get_blob_large_type_info(const data_type_info* blob_short_p, const page_access_specs* pas_p)
{
	data_type_info* dti_p = malloc(sizeof_tuple_data_type_info(2));
	if(dti_p == NULL)
		exit(-1);

	initialize_tuple_data_type_info(dti_p, "blob_large", 1, pas_p->page_size, 2);

	strcpy(dti_p->containees[0].field_name, "blob_prefix");
	dti_p->containees[0].al.type_info = (data_type_info*)blob_short_p;

	strcpy(dti_p->containees[1].field_name, "blob_extension");
	dti_p->containees[1].al.type_info = (data_type_info*)(&(pas_p->page_id_type_info));

	return dti_p;
}