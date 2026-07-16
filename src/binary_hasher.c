#include<tuplelargetypes/binary_hasher.h>

#include<tuplelargetypes/common_extended.h>
#include<tuplelargetypes/relative_positional_accessor.h>

uint64_t hash_tbn(const datum* uval, const data_type_info* dti, tuple_hasher* th, const blob_store_tuple_defs* bstd_p, const page_access_methods* pam_p, const void* transaction_id, int* abort_error, extension_reader_iterator_callback* callback)
{
	// if inline handle it here
	if(is_inline_type_info(dti))
		return hash_datum(uval, dti, th);

	// else it is extended type

	// hash the prefix
	{
		datum prefix;
		const data_type_info* prefix_dti;
		int valid = get_nested_containee_from_datum(&prefix, &prefix_dti, uval, dti, EXTENDED_PREFIX_POS_ACC);
		if(valid)
			hash_datum(&prefix, prefix_dti, th);
	}

	tuple_pointer extension_head = get_extension_head_for_extended_type(uval, dti, &(pam_p->pas));
	if(is_tuple_pointer_NULL(extension_head, &(pam_p->pas)))
		return th->hash;

	const char* buffer = NULL;
	uint32_t buffer_size = 0;

	if(callback) callback->extension_blob_read_begin_event(callback, uval, dti, pam_p);
	blob_store_read_iterator* bsri_p = get_new_blob_store_read_iterator(extension_head, 0, bstd_p, pam_p, transaction_id, abort_error);
	if(*abort_error)
	{
		if(callback) callback->extension_blob_read_ended_event(callback, uval, dti, pam_p);
		return 0;
	}

	while(1)
	{
		buffer = peek_in_blob(bsri_p, &buffer_size, transaction_id, abort_error);
		if(*abort_error)
		{
			delete_blob_store_read_iterator(bsri_p, transaction_id, abort_error);
			if(callback) callback->extension_blob_read_ended_event(callback, uval, dti, pam_p);
			return 0;
		}
		if(buffer_size == 0)
			break;

		tuple_hash_bytes(th, (const uint8_t*)buffer, buffer_size);

		// now skip buffer size number of bytes
		read_from_blob(bsri_p, NULL, buffer_size, transaction_id, abort_error);
		if(*abort_error)
		{
			delete_blob_store_read_iterator(bsri_p, transaction_id, abort_error);
			if(callback) callback->extension_blob_read_ended_event(callback, uval, dti, pam_p);
			return 0;
		}
	}

	delete_blob_store_read_iterator(bsri_p, transaction_id, abort_error);
	if(callback) callback->extension_blob_read_ended_event(callback, uval, dti, pam_p);
	if(*abort_error)
		return 0;

	return th->hash;
}