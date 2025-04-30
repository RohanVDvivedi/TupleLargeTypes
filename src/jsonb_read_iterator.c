#include<tuplelargetypes/jsonb_read_iterator.h>

// the ownership of key is transferred to the jb_acs
// since skip_size of jsonb_node is 32-bit integer, the depth of jsonb_accessor can never cross UINT32_MAX
// and this also forces that this function will never be called with jb_acs->keys_count == UINT32_MAX
static inline void push_to_jsonb_accessor(jsonb_accessor* jb_acs, int is_array_index, uint32_t index, uint32_t total_siblings_count, dstring key_consumed)
{
	if(jb_acs->keys_count == jb_acs->keys_capacity)
	{
		// assign an incremented value the next jb_acs->keys_capacity, ensuring to cap it at UINT32_MAX
		{
			uint64_t new_keys_capacity = UINT64_C(2) * (uint64_t)(jb_acs->keys_capacity) + UINT64_C(2);
			if(new_keys_capacity >= UINT32_MAX)
				new_keys_capacity = UINT32_MAX;
			jb_acs->keys_capacity = new_keys_capacity;
		}

		jb_acs->keys_list = realloc(jb_acs->keys_list, ((uint64_t)jb_acs->keys_capacity) * sizeof(jsonb_key));
		if(jb_acs->keys_list == NULL) // allocation failure
			exit(-1);
	}

	jb_acs->keys_list[jb_acs->keys_count++] = (jsonb_key){.is_array_index = is_array_index, .index = index, .total_siblings_count = total_siblings_count, .key = key_consumed};
}

static inline int pop_from_jsonb_accessor(jsonb_accessor* jb_acs)
{
	if(jb_acs->keys_count == 0)
		return 0;
	deinit_dstring(&(jb_acs->keys_list[--jb_acs->keys_count].key));
	return 1;
}

static inline int overwrite_top_key_in_jsonb_accessor(jsonb_accessor* jb_acs, dstring key_consumed)
{
	if(jb_acs->keys_count == 0)
		return 0;
	deinit_dstring(&(jb_acs->keys_list[jb_acs->keys_count-1].key));
	jb_acs->keys_list[jb_acs->keys_count-1].key = key_consumed;
	return 1;
}

static int are_incompatible_jsonb_N_json_accessors(const jsonb_accessor* jb_acs, const json_accessor* j_acs)
{
	// TODO
}

static int compare_jsonb_N_json_accessors(const jsonb_accessor* jb_acs, const json_accessor* j_acs, int* is_prefix)
{
	// TODO
}

int point_to_accessor_for_jsonb_read_iterator(jsonb_read_iterator* jri_p, const json_accessor* acs, const void* transaction_id, int* abort_error)
{
	// TODO
}

binary_read_iterator* get_cloned_iterator_for_jsonb_read_iterator(const jsonb_read_iterator* jri_p, const void* transaction_id, int* abort_error)
{
	return clone_binary_read_iterator(jri_p->bri_p, transaction_id, abort_error);
}