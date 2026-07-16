#include<tuplelargetypes/binary_write_iterator.h>

#include<tupleindexer/interface/page_access_methods.h>

#include<stdlib.h>

#include<tuplelargetypes/relative_positional_accessor.h>

binary_write_iterator* get_new_binary_write_iterator(void* tupl, const tuple_def* tpl_d, positional_accessor pos, uint64_t blob_store_root_page_id, tuple_pointer extension_tail, uint32_t bytes_to_be_written_to_prefix, const blob_store_tuple_defs* bstd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p)
{
	binary_write_iterator* bwi_p = malloc(sizeof(binary_write_iterator));
	if(bwi_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, pos);
	bwi_p->is_extended = is_extended_type_info(dti_p);

	bwi_p->tupl = tupl;
	bwi_p->tpl_d = tpl_d;
	bwi_p->pos = pos;

	bwi_p->blob_store_root_page_id = blob_store_root_page_id;

	bwi_p->extension_head = bwi_p->is_extended ? get_NULL_tuple_pointer(&(pam_p->pas)) : (tuple_pointer){};
	bwi_p->extension_tail = bwi_p->is_extended ? get_NULL_tuple_pointer(&(pam_p->pas)) : (tuple_pointer){};
	bwi_p->was_inline_OR_extended_head_modified = 0;

	bwi_p->bswi_p = NULL;

	bwi_p->bstd_p = bstd_p;
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
			set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, EMPTY_DATUM, UINT32_MAX);

			set_extension_head_for_extended_type(tupl, tpl_d, pos, &(pam_p->pas), get_NULL_tuple_pointer(&(pam_p->pas)));

			bwi_p->bytes_written_to_prefix = 0;
			bwi_p->bytes_to_be_written_to_prefix = bytes_to_be_written_to_prefix;
		}
		else // note down the number of bytes already written to the prefix
		{
			bwi_p->bytes_written_to_prefix = prefix.string_or_binary_size;
			bwi_p->bytes_to_be_written_to_prefix = bytes_to_be_written_to_prefix;

			// initialize extension pointers, only needed here
			{
				datum uval;
				get_value_from_element_from_tuple(&uval, tpl_d, pos, tupl);
				bwi_p->extension_head = get_extension_head_for_extended_type(&uval, dti_p, &(pam_p->pas));
				bwi_p->extension_tail = extension_tail;
			}

			// if the extension_head_page_id is not NULL, then we already wrote the prefix completely, so no more extension
			if(!is_tuple_pointer_NULL(bwi_p->extension_head, &(bwi_p->pam_p->pas)))
				bwi_p->bytes_to_be_written_to_prefix = bwi_p->bytes_written_to_prefix;
		}

		// limit the bytes_to_be_written_to_prefix, by the amount of bytes the tuple can allow us to expand it
		bwi_p->bytes_to_be_written_to_prefix = bwi_p->bytes_written_to_prefix +
			min(bwi_p->bytes_to_be_written_to_prefix - bwi_p->bytes_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl));
	}
	else
	{
		relative_positonal_accessor_set_from_relative(&child_relative_accessor, SELF);
		datum prefix;
		get_value_from_element_from_tuple(&prefix, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);

		bwi_p->bytes_written_to_prefix = prefix.string_or_binary_size;
		bwi_p->bytes_to_be_written_to_prefix = bytes_to_be_written_to_prefix;

		// limit the bytes_to_be_written_to_prefix, by the amount of bytes the tuple can allow us to expand it
		bwi_p->bytes_to_be_written_to_prefix = bwi_p->bytes_written_to_prefix +
			min(bwi_p->bytes_to_be_written_to_prefix - bwi_p->bytes_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl));
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);
	return bwi_p;
}

void delete_binary_write_iterator(binary_write_iterator* bwi_p, const void* transaction_id, int* abort_error)
{
	if(bwi_p->bswi_p != NULL)
		delete_blob_store_write_iterator(bwi_p->bswi_p, transaction_id, abort_error);
	free(bwi_p);
}

uint32_t append_to_binary_write_iterator(binary_write_iterator* bwi_p, const char* data, uint32_t data_size, const heap_table_notifier* notify_wrong_entry, const void* transaction_id, int* abort_error)
{
	int need_to_update_extension_head = 0;
	int need_to_update_extension_tail = 0;

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
			int expanded = expand_element_count_for_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, old_element_count, bytes_written_this_iteration, bytes_written_this_iteration);

			// copy data into it byte by byte
			/*
			point_to_i_th_child_position(&(child_relative_accessor.exact), old_element_count);
			for(uint32_t i = 0; i < bytes_written_this_iteration; i++, point_to_next_sibling_position(&(child_relative_accessor.exact)))
				set_element_in_tuple(bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl, &((datum){.uint_value = data[i]}), UINT32_MAX);
			*/
			// doing the same above thing but in more optimized way, this is illegal but fast
			if(expanded)
			{
				datum prefix;
				get_value_from_element_from_tuple(&prefix, bwi_p->tpl_d, child_relative_accessor.exact, bwi_p->tupl);
				memory_move((void*)(prefix.string_or_binary_value + old_element_count), data, bytes_written_this_iteration);
			}

			bwi_p->bytes_written_to_prefix += bytes_written_this_iteration;
			bwi_p->was_inline_OR_extended_head_modified = 1;
		}
		else if(bwi_p->is_extended)
		{
			if(bwi_p->bswi_p == NULL)
			{
				// open a new bswi
				bwi_p->bswi_p = get_new_blob_store_write_iterator(bwi_p->blob_store_root_page_id, bwi_p->extension_head, bwi_p->extension_tail, bwi_p->bstd_p, bwi_p->pam_p, bwi_p->pmm_p, transaction_id, abort_error);
				if(*abort_error)
				{
					deinitialize_relative_positional_accessor(&child_relative_accessor);
					return 0;
				}
			}

			// append to blob_store's blob
			bytes_written_this_iteration = append_to_tail_in_blob(bwi_p->bswi_p, data, data_size, NULL, notify_wrong_entry, transaction_id, abort_error);
			if(*abort_error)
			{
				deinitialize_relative_positional_accessor(&child_relative_accessor);
				return 0;
			}

			// whether to update chunk_ptrs to head and tail
			// only update head if it was previously NULL_PAGE_ID
			need_to_update_extension_head = need_to_update_extension_head || ((bytes_written_this_iteration > 0) && is_tuple_pointer_NULL(bwi_p->extension_head, &(bwi_p->pam_p->pas)));
			need_to_update_extension_tail = need_to_update_extension_tail || (bytes_written_this_iteration > 0);
		}

		if(bytes_written_this_iteration == 0)
			break;

		data += bytes_written_this_iteration;
		data_size -= bytes_written_this_iteration;
		bytes_written += bytes_written_this_iteration;
	}

	deinitialize_relative_positional_accessor(&child_relative_accessor);

	if(need_to_update_extension_head)
	{
		bwi_p->was_inline_OR_extended_head_modified = 1;
		bwi_p->extension_head = get_head_pointer_in_blob(bwi_p->bswi_p);
		set_extension_head_for_extended_type(bwi_p->tupl, bwi_p->tpl_d, bwi_p->pos, &(bwi_p->pam_p->pas), bwi_p->extension_head);
	}

	if(need_to_update_extension_tail)
		bwi_p->extension_tail = get_tail_pointer_in_blob(bwi_p->bswi_p);

	return bytes_written;
}