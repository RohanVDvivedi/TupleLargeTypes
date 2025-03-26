#include<binary_hasher.h>

#include<common_extended.h>
#include<relative_positional_accessor.h>

#define BUFFER_CAPACITY 1024

uint64_t hash_tbn(const tuple_def* tpl_d, const void* tupl, positional_accessor inline_accessor, tuple_hasher* th, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const void* transaction_id, int* abort_error)
{
	// check for the attribute being valid and not null
	{
		user_value uval;
		int valid = get_value_from_element_from_tuple(&uval, tpl_d, inline_accessor, tupl);
		if(!valid || is_user_value_NULL(&uval))
			return th->hash;
	}

	if(is_inline_type_info(get_type_info_for_element_from_tuple_def(tpl_d, inline_accessor)))
		return hash_tuple(tupl, tpl_d, &inline_accessor, th, 1);

	{
		relative_positional_accessor rpa;
		initialize_relative_positional_accessor(&rpa, &inline_accessor, 1);
		relative_positonal_accessor_set_from_relative(&rpa, STATIC_POSITION(0));

		hash_tuple(tupl, tpl_d, &(rpa.exact), th, 1);

		deinitialize_relative_positional_accessor(&rpa);
	}

	uint64_t extension_head_page_id = get_extension_head_page_id_for_extended_type(tupl, tpl_d, inline_accessor, &(pam_p->pas));
	if(extension_head_page_id == pam_p->pas.NULL_PAGE_ID)
		return th->hash;

	char buffer[BUFFER_CAPACITY];
	uint32_t buffer_size;

	worm_read_iterator* wri_p = get_new_worm_read_iterator(extension_head_page_id, wtd_p, pam_p, transaction_id, abort_error);
	if(*abort_error)
		return 0;

	while(1)
	{
		buffer_size = read_from_worm(wri_p, buffer, BUFFER_CAPACITY, transaction_id, abort_error);
		if(*abort_error)
		{
			delete_worm_read_iterator(wri_p, transaction_id, abort_error);
			return 0;
		}
		if(buffer_size == 0)
			break;

		tuple_hash_bytes(th, (const uint8_t*)buffer, buffer_size);
	}

	delete_worm_read_iterator(wri_p, transaction_id, abort_error);
	if(*abort_error)
		return 0;

	return th->hash;
}