#include<tuplelargetypes/binary_hasher.h>

#include<tuplelargetypes/common_extended.h>
#include<tuplelargetypes/relative_positional_accessor.h>

uint64_t hash_tbn(const datum* uval, const data_type_info* dti, tuple_hasher* th, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const void* transaction_id, int* abort_error)
{
	// if inline handle it here
	if(is_inline_type_info(dti))
		return hash_datum(uval, dti, th);

	// else it is extended type

	// hash the prefix
	{
		datum prefix;
		const data_type_info* prefix_dti = get_data_type_info_for_containee_of_container_without_data(dti, EXTENDED_PREFIX_POS_VAL);
		if((prefix_dti != NULL) && get_containee_for_datum(&prefix, uval, dti, EXTENDED_PREFIX_POS_VAL))
			hash_datum(&prefix, dti, th);
	}

	uint64_t extension_head_page_id = get_extension_head_page_id_for_extended_type(uval, dti, &(pam_p->pas));
	if(extension_head_page_id == pam_p->pas.NULL_PAGE_ID)
		return th->hash;

	const char* buffer = NULL;
	uint32_t buffer_size = 0;

	worm_read_iterator* wri_p = get_new_worm_read_iterator(extension_head_page_id, wtd_p, pam_p, transaction_id, abort_error);
	if(*abort_error)
		return 0;

	while(1)
	{
		buffer = peek_in_worm(wri_p, &buffer_size, transaction_id, abort_error);
		if(*abort_error)
		{
			delete_worm_read_iterator(wri_p, transaction_id, abort_error);
			return 0;
		}
		if(buffer_size == 0)
			break;

		tuple_hash_bytes(th, (const uint8_t*)buffer, buffer_size);

		// now skip buffer size number of bytes
		read_from_worm(wri_p, NULL, buffer_size, transaction_id, abort_error);
		if(*abort_error)
		{
			delete_worm_read_iterator(wri_p, transaction_id, abort_error);
			return 0;
		}
	}

	delete_worm_read_iterator(wri_p, transaction_id, abort_error);
	if(*abort_error)
		return 0;

	return th->hash;
}