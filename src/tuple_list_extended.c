#include<tuplelargetypes/tuple_list_extended.h>

#include<stdlib.h>

int is_tuple_list_extended_type_info(const data_type_info* dti_p)
{
	if( is_prefix_of_dstring(&get_dstring_pointing_to_cstring(dti_p->type_name), &get_dstring_pointing_to_literal_cstring(TUPLE_LIST_TYPE_PREFIX)) &&
		is_suffix_of_dstring(&get_dstring_pointing_to_cstring(dti_p->type_name), &get_dstring_pointing_to_literal_cstring(EXTENDED_TYPE_SUFFIX)) )
		return 1;

	return 0;
}

data_type_info* get_tuple_list_extended_type_info(const char* extension_sub_type, uint32_t max_size, uint32_t inline_size, const page_access_specs* pas_p)
{
	data_type_info* dti_p = malloc(sizeof_tuple_data_type_info(2));
	if(dti_p == NULL)
		exit(-1);

	// the binary_inline controls the total size so we allow the binary_extended to be atmost page_size bytes large
	initialize_tuple_data_type_info(dti_p, TUPLE_LIST_TYPE_PREFIX "_" EXTENDED_TYPE_SUFFIX, 1, max_size, 2);
	if(extension_sub_type != NULL)
		sprintf(dti_p->type_name, TUPLE_LIST_TYPE_PREFIX "_{%s}_" EXTENDED_TYPE_SUFFIX, extension_sub_type);

	strcpy(dti_p->containees[0].field_name, "tuple_list_prefix");
	{
		data_type_info* tuple_list_inline_p = malloc(sizeof(data_type_info));
		if(tuple_list_inline_p == NULL)
			exit(-1);

		(*tuple_list_inline_p) = get_variable_length_binary_type(TUPLE_LIST_TYPE_PREFIX "_" INLINE_TYPE_SUFFIX, inline_size);

		dti_p->containees[0].al.type_info = (data_type_info*)tuple_list_inline_p;
	}

	strcpy(dti_p->containees[1].field_name, "tuple_list_extension");
	dti_p->containees[1].al.type_info = (data_type_info*)(&(pas_p->tuple_pointer_type_info));

	return dti_p;
}