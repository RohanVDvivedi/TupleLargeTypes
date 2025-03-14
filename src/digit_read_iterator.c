#include<digit_read_iterator.h>

#include<page_access_methods.h>

#include<stdlib.h>

#include<relative_positional_accessor.h>

#include<digit_iterator_commons.h>

digit_read_iterator* get_new_digit_read_iterator(const void* tupl, const tuple_def* tpl_d, positional_accessor inline_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p)
{
	digit_read_iterator* dri_p = malloc(sizeof(digit_read_iterator));
	if(dri_p == NULL)
		exit(-1);

	const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor);
	dri_p->is_inline = is_inline_type_info(dti_p);

	dri_p->tupl = tupl;
	dri_p->tpl_d = tpl_d;
	dri_p->inline_accessor = inline_accessor;

	dri_p->digits_read_from_prefix = 0;

	dri_p->wri_p = NULL;

	dri_p->wtd_p = wtd_p;
	dri_p->pam_p = pam_p;

	return dri_p;
}

void delete_digit_read_iterator(digit_read_iterator* dri_p, const void* transaction_id, int* abort_error)
{
	if(dri_p->wri_p != NULL)
		delete_worm_read_iterator(dri_p->wri_p, transaction_id, abort_error);
	free(dri_p);
}

digit_read_iterator* clone_digit_read_iterator(digit_read_iterator* dri_p, const void* transaction_id, int* abort_error)
{
	digit_read_iterator* clone_p = malloc(sizeof(digit_read_iterator));
	if(clone_p == NULL)
		exit(-1);

	(*clone_p) = (*dri_p);

	if((!clone_p->is_inline) && (dri_p->wri_p != NULL)) // if is_large && wri_p != NULL then a wri_p clone is necessary
	{
		clone_p->wri_p = clone_worm_read_iterator(dri_p->wri_p, transaction_id, abort_error);
		if(*abort_error)
		{
			free(clone_p);
			return NULL;
		}
	}

	return clone_p;
}