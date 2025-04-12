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
extern jsonb_node jsonb_true;
extern jsonb_node jsonb_false;

// below 2 functions clone internally
jsonb_node* get_jsonb_string_node(const dstring* str);
jsonb_node* get_jsonb_numeric_node(const materialized_numeric* m);

jsonb_node* get_jsonb_array_node(uint32_t capacity);
jsonb_node* get_jsonb_object_node();

// fails if the size exceeds UINT32_MAX
// node_p and not its clone is pushed at the end of the arraylist
int push_in_jsonb_array_node(jsonb_node* array_p, jsonb_node* node_p);

// node_p and not its clone is pushed at the end of the arraylist
// key is cloned for internal use
int put_in_jsonb_object_node(jsonb_node* object_p, const dstring* key, jsonb_node* node_p);

// computes skip sizes all the way from root to leaf
// you may use this json node only after this function succeeds
// returns 0, if any of the skip_sizes overflows
int finalize_jsonb(jsonb_node* node_p);

void delete_jsonb_node(jsonb_node* node_p);

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