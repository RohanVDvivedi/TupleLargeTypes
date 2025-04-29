#include<tuplelargetypes/jsonb_read_iterator.h>

// the ownership of key is transferred to the jb_acs
static inline int push_to_jsonb_accessor(jsonb_accessor* jb_acs, int is_array_index, uint32_t index, uint32_t total_siblings_count, dstring key_consumed)
{
	// TODO
}

static inline int pop_from_jsonb_accessor(jsonb_accessor* jb_acs)
{
	// TODO
}

static inline int overwrite_top_key_in_jsonb_accessor(jsonb_accessor* jb_acs, dstring key_consumed)
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