#include<tuplelargetypes/tuple_list_extended.h>

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

int can_tuple_be_inserted_in_tuple_list_extended(const tuple_def* tpl_d)
{
	uint32_t max_absolute_depth = 10;
	positional_accessor absolute_position = {.positions = malloc(sizeof(uint32_t) * max_absolute_depth), .positions_length = 0};
	if(absolute_position.positions == NULL)
		exit(-1);

	int can_be_inserted = 1;

	while(can_be_inserted)
	{
		const data_type_info* dti = get_type_info_for_element_from_tuple_def(tpl_d, absolute_position);

		{
			const data_type_info* parent_dti = NULL;
			{
				positional_accessor parent_position = absolute_position;
				if(point_to_parent_position(&parent_position))
					parent_dti = get_type_info_for_element_from_tuple_def(tpl_d, parent_position);
			}
			if((dti == NULL) || (parent_dti != NULL && (parent_dti->type == ARRAY || parent_dti->type == STRING || parent_dti->type == BLOB) && absolute_position.positions[absolute_position.positions_length-1] == 1))
			{
				if((absolute_position.positions_length >= 2) && point_to_next_uncle_position(&absolute_position))
					continue;
				else
					break;
			}
		}

		// analyze dti and user_value
		can_be_inserted = can_be_inserted && (!is_extended_type_info(dti));
		if(can_be_inserted == 0)
			break;

		// default way to go next
		if(is_container_type_info(dti))
		{
			if(absolute_position.positions_length == max_absolute_depth)
			{
				max_absolute_depth *= 2;
				absolute_position.positions = realloc(absolute_position.positions, sizeof(uint32_t) * max_absolute_depth);
				if(absolute_position.positions == NULL)
					exit(-1);
			}
			point_to_first_child_position(&absolute_position);
			continue;
		}
		else
		{
			if(absolute_position.positions_length > 0)
			{
				point_to_next_sibling_position(&absolute_position);
				continue;
			}
			else
				break;
		}
	}

	free(absolute_position.positions);
	return can_be_inserted;
}