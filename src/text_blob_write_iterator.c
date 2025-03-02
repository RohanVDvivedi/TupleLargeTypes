#include<text_blob_write_iterator.h>

#include<stdlib.h>

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

	tbwi_p->wai = NULL;

	tbwi_p->wtd_p = wtd_p;
	tbwi_p->pam_p = pam_p;
	tbwi_p->pmm_p = pmm_p;

	return tbwi_p;
}

void delete_text_blob_write_iterator(text_blob_write_iterator* tbwi_p, const void* transaction_id, int* abort_error);

uint32_t append_to_text_blob(text_blob_write_iterator* tbwi_p, const char* data, uint32_t data_size, const void* transaction_id, int* abort_error);