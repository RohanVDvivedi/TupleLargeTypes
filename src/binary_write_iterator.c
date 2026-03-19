#include<tuplelargetypes/binary_write_iterator.h>

#include<tupleindexer/interface/page_access_methods.h>

#include<stdlib.h>

#include<tuplelargetypes/relative_positional_accessor.h>

binary_write_iterator* get_new_binary_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor pos, uint32_t bytes_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p)
{
	binary_write_iterator* bwi_p = malloc(sizeof(binary_write_iterator));
	if(bwi_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, pos);
	bwi_p->is_extended = is_extended_type_info(dti_p);

	bwi_p->tupl = tupl;
	bwi_p->tpl_d = tpl_d;
	bwi_p->pos = pos;

	bwi_p->wai_p = NULL;

	bwi_p->wtd_p = wtd_p;
	bwi_p->pam_p = pam_p;
	bwi_p->pmm_p = pmm_p;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(bwi_p->pos), 2);

	if(bwi_p->is_extended)
	{
		relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENDED_PREFIX_POS_ACC);
		datum prefix;
		get_value_from_element_from_tuple(&prefix, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);

		if(is_datum_NULL(&prefix))
		{
			relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENDED_PREFIX_POS_ACC);
			set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, EMPTY_DATUM, UINT32_MAX);

			relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENDED_HEAD_PAGE_ID_POS_ACC);
			set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, &((datum){.uint_value = bwi_p->pam_p->pas.NULL_PAGE_ID}), UINT32_MAX);

			bwi_p->bytes_written_to_prefix = 0;
			bwi_p->bytes_to_be_written_to_prefix = bytes_to_be_written_to_prefix;
		}
		else // note down the number of bytes already written to the prefix
		{
			bwi_p->bytes_written_to_prefix = prefix.string_or_binary_size;
			bwi_p->bytes_to_be_written_to_prefix = bytes_to_be_written_to_prefix;

			relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENDED_HEAD_PAGE_ID_POS_ACC);
			datum extension_head_page_id;
			get_value_from_element_from_tuple(&extension_head_page_id, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);

			// if the extension_head_page_id is not NULL, then we already wrote the prefix completely
			if(!is_datum_NULL(&extension_head_page_id) && extension_head_page_id.uint_value != bwi_p->pam_p->pas.NULL_PAGE_ID)
				bwi_p->bytes_to_be_written_to_prefix = bwi_p->bytes_written_to_prefix;
		}
	}

	// limit the bytes_to_be_written_to_prefix, by the amount of bytes the tuple can allow us to expand it
	if(bwi_p->is_extended)
	{
		relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENDED_PREFIX_POS_ACC);
		bwi_p->bytes_to_be_written_to_prefix = bwi_p->bytes_written_to_prefix +
			min(bwi_p->bytes_to_be_written_to_prefix - bwi_p->bytes_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl));
	}
	else
	{
		relative_positonal_accessor_set_from_relative(&child_relative_accessor, SELF);
		bwi_p->bytes_to_be_written_to_prefix = bwi_p->bytes_written_to_prefix +
			min(bwi_p->bytes_to_be_written_to_prefix - bwi_p->bytes_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl));
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);
	return bwi_p;
}

void delete_binary_write_iterator(binary_write_iterator* bwi_p, const void* transaction_id, int* abort_error)
{
	if(bwi_p->wai_p != NULL)
		delete_worm_append_iterator(bwi_p->wai_p, transaction_id, abort_error);
	free(bwi_p);
}

uint32_t append_to_binary_write_iterator(binary_write_iterator* bwi_p, const char* data, uint32_t data_size, const void* transaction_id, int* abort_error)
{
	if(data_size == 0)
		return 0;

	relative_positional_accessor child_relative_accessor;
	initialize_relative_positional_accessor(&child_relative_accessor, &(bwi_p->pos), 2);

	uint32_t bytes_written = 0;

	while(data_size > 0)
	{
		uint32_t bytes_written_this_iteration = 0;

		if(bwi_p->bytes_written_to_prefix < bwi_p->bytes_to_be_written_to_prefix)
		{
			bytes_written_this_iteration = min(data_size, bwi_p->bytes_to_be_written_to_prefix - bwi_p->bytes_written_to_prefix);

			// grab old_element_count and expand the container
			if(bwi_p->is_extended)
				relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENDED_PREFIX_POS_ACC);
			else
				relative_positonal_accessor_set_from_relative(&child_relative_accessor, SELF);
			uint32_t old_element_count = get_element_count_for_element_from_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);
			expand_element_count_for_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, old_element_count, bytes_written_this_iteration, bytes_written_this_iteration);

			// copy data into it byte by byte
			point_to_i_th_child_position(&(child_relative_accessor.exact), old_element_count);
			for(uint32_t i = 0; i < bytes_written_this_iteration; i++)
			{
				point_to_next_sibling_position(&(child_relative_accessor.exact));
				set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, &((datum){.uint_value = data[i]}), UINT32_MAX);
			}

			bwi_p->bytes_written_to_prefix += bytes_written_this_iteration;
		}
		else if(bwi_p->is_extended)
		{
			if(bwi_p->wai_p == NULL)
			{
				// read extension_head_page_id
				uint64_t extension_head_page_id;
				{
					relative_positonal_accessor_set_from_relative(&child_relative_accessor, EXTENDED_HEAD_PAGE_ID_POS_ACC);
					datum extension_head_page_id_uval;
					get_value_from_element_from_tuple(&extension_head_page_id_uval, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);
					extension_head_page_id = extension_head_page_id_uval.uint_value;
				}

				// if it is NULL_PAGE_ID, then create a new worm and set it in the attribute beside prefix
				if(extension_head_page_id == bwi_p->pam_p->pas.NULL_PAGE_ID)
				{
					extension_head_page_id = get_new_worm(1, bwi_p->pam_p->pas.NULL_PAGE_ID, bwi_p->wtd_p, bwi_p->pam_p, bwi_p->pmm_p, transaction_id, abort_error);
					if(*abort_error)
					{
						deinitialize_relative_positional_accessor(&child_relative_accessor);
						return 0;
					}
					set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, &((datum){.uint_value = extension_head_page_id}), UINT32_MAX);
				}

				// open a new wai
				bwi_p->wai_p = get_new_worm_append_iterator(extension_head_page_id, bwi_p->wtd_p, bwi_p->pam_p, bwi_p->pmm_p, transaction_id, abort_error);
				if(*abort_error)
				{
					deinitialize_relative_positional_accessor(&child_relative_accessor);
					return 0;
				}
			}

			// append to worm
			bytes_written_this_iteration = append_to_worm(bwi_p->wai_p, data, data_size, NULL, NULL, transaction_id, abort_error);
			if(*abort_error)
			{
				deinitialize_relative_positional_accessor(&child_relative_accessor);
				return 0;
			}
		}

		if(bytes_written_this_iteration == 0)
			break;

		data += bytes_written_this_iteration;
		data_size -= bytes_written_this_iteration;
		bytes_written += bytes_written_this_iteration;
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);
	return bytes_written;
}