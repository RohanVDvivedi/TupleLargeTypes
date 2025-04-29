#ifndef JSONB_READ_ITERATOR_H
#define JSONB_READ_ITERATOR_H

#include<stdint.h>
#include<cutlery/dstring.h>

// similar to json_key
typedef struct jsonb_key jsonb_key;
struct jsonb_key
{
	int is_array_index; // if this key indexes into an element

	uint32_t index; // index of this element into a jsonb_array or jsonb_object

	uint32_t total_siblings_count; // index < total_siblings_count -> if out of bounds then pop this element

	dstring key; // valid only if !(not) is_array_index
};

typedef struct jsonb_accessor jsonb_accessor;
struct jsonb_accessor
{
	uint32_t keys_capacity; // number of available elements at keys_list

	uint32_t keys_count; // number of elements at keys_list

	jsonb_key* keys_list;
};

#define EMPTY_JSONB_ACCESSOR (jsonb_accessor){.keys_capacity = 0, .keys_count = 0, .keys_list = NULL}

// the above two structures just assist in completing the valued curr_acs attribute below

#include<tuplelargetypes/binary_read_iterator.h>

typedef struct jsonb_read_iterator jsonb_read_iterator;
struct jsonb_read_iterator
{
	jsonb_accessor curr_acs; // contains dynamically allocated keys for the current position

	binary_read_iterator* bri_p; // actual position in the iterator
};

#define init_jsonb_read_iterator(bri_p_v) (jsonb_read_iterator){.curr_acs = EMPTY_JSONB_ACCESSOR, .bri_p = bri_p_v}

#include<jsonparser/json_accessor.h>

// returns 1 only if the json_read_iterator now exactly points to the acs accessor passed
int point_to_accessor_for_jsonb_read_iterator(jsonb_read_iterator* jri_p, const json_accessor* acs, const void* transaction_id, int* abort_error);

binary_read_iterator* get_cloned_iterator_for_jsonb_read_iterator(const jsonb_read_iterator* jri_p, const void* transaction_id, int* abort_error);

#define deinit_jsonb_read_iterator(jri_p) if(jri_p->curr_acs.keys_list != NULL){free(jri_p->curr_acs.keys_list);}

#endif