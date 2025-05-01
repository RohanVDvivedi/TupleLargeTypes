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

// checks if the accessors are validly accessible on the same object
static int are_compatible_jsonb_N_json_accessors(const jsonb_accessor* jb_acs, const json_accessor* j_acs)
{
	for(uint32_t i = 0; i < min(jb_acs->keys_count, j_acs->keys_length); i++)
	{
		jsonb_key* k1 = jb_acs->keys_list + i;
		json_key* k2 = j_acs->keys_list + i;

		// if they attempt to index one with an array_index and another one with object_key, then this check fails
		if(k1->is_array_index != k2->is_array_index)
			return 0;

		// if they point to different chldren within an array or an object, then break
		if(
			(k1->is_array_index && k1->index != k2->index) ||
			(!(k1->is_array_index) && 0 != compare_dstring(&(k1->key), &(k2->key)))
		)
			break;
	}

	return 1;
}

// comparison works only with compatible accessors, i.e. above function returning 1
static int compare_jsonb_N_json_accessors(const jsonb_accessor* jb_acs, const json_accessor* j_acs, int* is_prefix)
{
	int cmp = 0;
	(*is_prefix) = 0;

	for(uint32_t i = 0; i < min(jb_acs->keys_count, j_acs->keys_length) && (cmp == 0); i++)
	{
		jsonb_key* k1 = jb_acs->keys_list + i;
		json_key* k2 = j_acs->keys_list + i;

		if(k1->is_array_index)
			cmp = compare_numbers(k1->index, k2->index);
		else
			cmp = compare_dstring(&(k1->key), &(k2->key));
	}

	if(cmp == 0) // min length comparison returned 0, i.e. they have a common prefix, with one being a prefix of another
	{
		cmp = compare_numbers(jb_acs->keys_count, j_acs->keys_length); // shorted one comes before longer one
		if(cmp == 0) // equal lengths, so both are prefixes of one another
			(*is_prefix) = 1 | 2;
		else if(cmp == -1) // 1st element is smaller, so it is prefix of the second one
			(*is_prefix) = 1;
		else // else second one is the prefic of the first one
			(*is_prefix) = 2;
	} // else we found a mistmatching element, so is_prefix = 0, none is prefix of another one

	return cmp;
}

#include<tuplelargetypes/jsonb_node.h>

// if no bytes could be peeked, then (*end_reached) = 1 will be set
static inline jsonb_type get_type_for_next_jsonb_read(const jsonb_read_iterator* jri_p, int* end_reached, const void* transaction_id, int* abort_error)
{
	(*end_reached) = 0;
	uint32_t data_size = 0;
	const char* data = peek_in_binary_read_iterator(jri_p->bri_p, &data_size, transaction_id, abort_error);
	if(*abort_error) // we have to return something
		return JSONB_NULL;

	if(data_size == 0)
	{
		(*end_reached) = 1;
		return JSONB_NULL;
	}

	return data[0];
}

int point_to_accessor_for_jsonb_read_iterator(jsonb_read_iterator* jri_p, const json_accessor* acs, const void* transaction_id, int* abort_error)
{
	// TODO
}

binary_read_iterator* get_cloned_iterator_for_jsonb_read_iterator(const jsonb_read_iterator* jri_p, const void* transaction_id, int* abort_error)
{
	return clone_binary_read_iterator(jri_p->bri_p, transaction_id, abort_error);
}