#include<numeric_large.h>

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
	// TODO
}