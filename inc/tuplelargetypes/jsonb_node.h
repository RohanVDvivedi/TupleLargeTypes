#ifndef JSONB_NODE_H
#define JSONB_NODE_H

#include<cutlery/dstring.h>
#include<cutlery/bst.h>
#include<cutlery/arraylist.h>

#include<tuplelargetypes/materialized_numeric.h>

typedef enum jsonb_type jsonb_type;
enum jsonb_type
{
	JSONB_NULL = 0,
	JSONB_TRUE = 1,
	JSONB_FALSE = 2,
	JSONB_STRING = 3,
	JSONB_NUMERIC = 4,
	JSONB_ARRAY = 5,
	JSONB_OBJECT = 6,
};

typedef struct jsonb_node jsonb_node;
struct jsonb_node
{
	jsonb_type type; // NULL, true and false encapsulated here, and there will be static jsonb_node for that

	uint32_t skip_size; // these are the bytes that need to be skipped if you want to completely skip the current element
	// it does not include the space occupied by the type information and then skip_size itself
	// it is calculated by the finalize_jsonb function and must be used only after this call

	union
	{
		dstring jsonb_string;
		materialized_numeric jsonb_numeric;
		struct
		{
			bst jsonb_object;
			// bst does not store element_count hence the need
			uint32_t element_count; // number of jsonb_object_entry in jsonb_object OR number of jsonb_node in jsonb_array
		};
		arraylist jsonb_array;
	};
};

typedef struct jsonb_object_entry jsonb_object_entry;
struct jsonb_object_entry
{
	dstring key;
	jsonb_node* value;

	bstnode jsonb_object_embed_node;
};

// a JSONB_NULL node itself may be a NULL pointer
extern jsonb_node jsonb_null;
extern jsonb_node jsonb_true;
extern jsonb_node jsonb_false;

// below 2 functions clone internally
jsonb_node* new_jsonb_string_node(const dstring* str);
jsonb_node* new_jsonb_numeric_node(const materialized_numeric* m);

// below 2 functions do not clone the parameters, instead a shallow copy of the passed parameters is made
// essentially stealing the resources
jsonb_node* new_jsonb_string_node2(dstring str_consumed);
jsonb_node* new_jsonb_numeric_node2(materialized_numeric m_consumed);

jsonb_node* new_jsonb_array_node(uint32_t capacity);
jsonb_node* new_jsonb_object_node();

// fails if the size exceeds UINT32_MAX
// node_p and not its clone is pushed at the end of the arraylist
int push_in_jsonb_array_node(jsonb_node* array_p, jsonb_node* node_p);

// node_p and not its clone is pushed at the right location in the jsonb_object bst
// key is cloned for internal use
int put_in_jsonb_object_node(jsonb_node* object_p, const dstring* key, jsonb_node* node_p);
// the below variant of the above function, transfers the key_consumed's ownership using a shallow copy
int put_in_jsonb_object_node2(jsonb_node* object_p, dstring key_consumed, jsonb_node* node_p);

#include<jsonparser/json_accessor.h>
jsonb_node* fetch_jsonb_from_jsonb(const jsonb_node* node_p, json_accessor acs, int* non_existing);

// computes skip sizes all the way from root to leaf
// you may use this json node only after this function succeeds (after skip_sizes are set accordingly)
// returns 0, if any of the skip_sizes or total_size overflows
int finalize_jsonb(jsonb_node* node_p, uint32_t* total_size);

void delete_jsonb_node(jsonb_node* node_p);

int are_equal_jsonb(const jsonb_node* n1_p, const jsonb_node* n2_p);

// you need to call finalize_jsonb on the return value of the below function before serializing it
jsonb_node* clone_jsonb(const jsonb_node* node_p);

void print_jsonb(const jsonb_node* node_p, uint32_t tabs);

#endif

/*
The first byte stores the type information of the object
It will be consumed by the parse function
Rest bytes will be consumed by the dedicated function
	0 -> NULL  -> no futher bytes -> skip_size = 0
	1 -> true  -> no further bytes -> skip_size = 0
	2 -> false -> no further bytes -> skip_size = 0
	3 -> string -> 4 byte skip_size, and then the bytes
	4 -> numeric -> 4 byte skip_size (always 3 + 3*Ndigits), 1 byte sign bits, 2 bytes exponent, then digits
	5 -> json array -> 4 byte skip_size (>= 4), 4 byte element count, then elements
	6 -> json object -> 4 byte skip_size (>= 4), 4 byte element count, then keys (strings without type information) ordered lexicographically and values alternatively

	to skip read size information and then again skip that many bytes
*/