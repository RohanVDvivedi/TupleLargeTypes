#include<numeric_large.h>

#include<stdlib.h>

int is_numeric_short_type_info(const data_type_info* numeric_short_p)
{
	return strcmp(numeric_short_p->type_name, "numeric_short") == 0;
}

int is_numeric_large_type_info(const data_type_info* numeric_large_p)
{
	return strcmp(numeric_large_p->type_name, "numeric_large") == 0;
}

data_type_info* get_numeric_short_type_info(uint32_t max_size)
{
	// TODO
}

data_type_info* get_numeric_large_type_info(const data_type_info* numeric_short_p, const page_access_specs* pas_p)
{
	data_type_info* dti_p = malloc(sizeof_tuple_data_type_info(2));
	if(dti_p == NULL)
		exit(-1);

	// the numeric_short controls the total size so we allow the blob_large to be atmost page_size bytes large
	initialize_tuple_data_type_info(dti_p, "numeric_large", 1, pas_p->page_size, 2);

	strcpy(dti_p->containees[0].field_name, "numeric_prefix");
	dti_p->containees[0].al.type_info = (data_type_info*)numeric_short_p;

	strcpy(dti_p->containees[1].field_name, "numeric_extension");
	dti_p->containees[1].al.type_info = (data_type_info*)(&(pas_p->page_id_type_info));

	return dti_p;
}