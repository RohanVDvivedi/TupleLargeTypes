#include<text_blob_read_iterator.h>

#include<stdlib.h>

text_blob_read_iterator* get_new_text_blob_read_iterator(void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p)
{
	text_blob_read_iterator* tbri_p = malloc(sizeof(text_blob_read_iterator));
	if(tbri_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	tbri_p->is_short = is_text_short_type_info(dti_p) || is_blob_short_type_info(dti_p);

	if(tbri_p->is_short)
		get_value_from_element_from_tuple(&(tbri_p->prefix), tpl_d, inline_accessor, tupl);
	else
	{
		positional_accessor child_inline_accessor = {.positions_length = 0, .positions = malloc(sizeof(uint32_t) * (inline_accessor.positions_length + 1))};
		if(child_inline_accessor.positions == NULL)
			exit(-1);
		append_positions(&child_inline_accessor, inline_accessor);

		append_positions(&child_inline_accessor, STATIC_POSITION(0));
		get_value_from_element_from_tuple(&(tbri_p->prefix), tpl_d, child_inline_accessor, tupl);
		child_inline_accessor.positions_length--;

		user_value worm_head_page_id;
		append_positions(&child_inline_accessor, STATIC_POSITION(1));
		get_value_from_element_from_tuple(&worm_head_page_id, tpl_d, child_inline_accessor, tupl);
		child_inline_accessor.positions_length--;
		tbri_p->extension_head_page_id = worm_head_page_id.uint_value;

		free(child_inline_accessor.positions);
	}

	tbri_p->bytes_read_from_prefix = 0;

	tbri_p->wri = NULL;

	tbri_p->wtd_p = wtd_p;
	tbri_p->pam_p = pam_p;

	return tbri_p;
}

void delete_text_blob_read_iterator(text_blob_read_iterator* tbri_p, const void* transaction_id, int* abort_error)
{
	if(tbri_p->wri != NULL)
		delete_worm_read_iterator(tbri_p->wri, transaction_id, abort_error);
	free(tbri_p);
}

text_blob_read_iterator* clone_text_blob_read_iterator(text_blob_read_iterator* tbri_p, const void* transaction_id, int* abort_error);

uint32_t read_from_text_blob(text_blob_read_iterator* tbri_p, void* data, uint32_t data_size, const void* transaction_id, int* abort_error);