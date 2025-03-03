#include<text_blob_write_iterator.h>

#include<stdlib.h>
#include<page_access_methods.h>

text_blob_write_iterator* get_new_text_blob_write_iterator(void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor, uint32_t bytes_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p)
{
	text_blob_write_iterator* tbwi_p = malloc(sizeof(text_blob_write_iterator));
	if(tbwi_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	tbwi_p->is_short = is_text_short_type_info(dti_p) || is_blob_short_type_info(dti_p);

	tbwi_p->tupl = tupl;
	tbwi_p->tpl_d = tpl_d;
	tbwi_p->inline_accessor = inline_accessor;

	tbwi_p->bytes_to_be_written_to_prefix = bytes_to_be_written_to_prefix;
	tbwi_p->bytes_written_to_prefix = 0;

	tbwi_p->wai_p = NULL;

	tbwi_p->wtd_p = wtd_p;
	tbwi_p->pam_p = pam_p;
	tbwi_p->pmm_p = pmm_p;

	return tbwi_p;
}

void delete_text_blob_write_iterator(text_blob_write_iterator* tbwi_p, const void* transaction_id, int* abort_error)
{
	if(tbwi_p->wai_p != NULL)
		delete_worm_append_iterator(tbwi_p->wai_p, transaction_id, abort_error);
	free(tbwi_p);
}

uint32_t append_to_text_blob(text_blob_write_iterator* tbwi_p, const char* data, uint32_t data_size, const void* transaction_id, int* abort_error)
{
	if(data_size == 0)
		return 0;

	// initialization
	// if the attribute is NULL, set it to EMPTY_USER_VALUE
	{
		user_value attr;
		get_value_from_element_from_tuple(&attr, tbwi_p->tpl_d, tbwi_p->inline_accessor, tbwi_p->tupl);
		if(is_user_value_NULL(&attr))
			set_element_in_tuple(tbwi_p->tpl_d, tbwi_p->inline_accessor, tbwi_p->tupl, EMPTY_USER_VALUE, UINT32_MAX);
	}

	// if the prefix is NULL in a large text or blob, set it to EMPTY_USER_VALUE and then bytes_to_be_written_to_prefix = min(bytes_to_be_written_to_prefix, max_size_increment_allowed);, then set the blob_extension to NULL_PAGE_ID
	positional_accessor child_inline_accessor = {};
	if(!(tbwi_p->is_short))
	{
		child_inline_accessor = (positional_accessor){.positions_length = 0, .positions = malloc(sizeof(uint32_t) * (tbwi_p->inline_accessor.positions_length + 2))};
		if(child_inline_accessor.positions == NULL)
			exit(-1);
		append_positions(&child_inline_accessor, tbwi_p->inline_accessor);

		append_positions(&child_inline_accessor, STATIC_POSITION(0));
		user_value prefix;
		get_value_from_element_from_tuple(&prefix, tbwi_p->tpl_d, child_inline_accessor, tbwi_p->tupl);
		int reset = 0;
		if(is_user_value_NULL(&prefix))
		{
			reset = 1;
			set_element_in_tuple(tbwi_p->tpl_d, child_inline_accessor, tbwi_p->tupl, EMPTY_USER_VALUE, UINT32_MAX);
		}
		tbwi_p->bytes_to_be_written_to_prefix = min(tbwi_p->bytes_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(tbwi_p->tpl_d, child_inline_accessor, tbwi_p->tupl));
		child_inline_accessor.positions_length = tbwi_p->inline_accessor.positions_length;
		if(reset)
		{
			append_positions(&child_inline_accessor, STATIC_POSITION(1));
			set_element_in_tuple(tbwi_p->tpl_d, child_inline_accessor, tbwi_p->tupl, &((user_value){.uint_value = tbwi_p->pam_p->pas.NULL_PAGE_ID}), UINT32_MAX);
			child_inline_accessor.positions_length = tbwi_p->inline_accessor.positions_length;
		}
	}
	else
	{
		child_inline_accessor = tbwi_p->inline_accessor;
		tbwi_p->bytes_to_be_written_to_prefix = min(tbwi_p->bytes_to_be_written_to_prefix, get_max_size_increment_allowed_for_element_in_tuple(tbwi_p->tpl_d, child_inline_accessor, tbwi_p->tupl));
	}

	uint32_t bytes_written = 0;

	// TODO

	if(!(tbwi_p->is_short))
		free(child_inline_accessor.positions);

	return bytes_written;
}