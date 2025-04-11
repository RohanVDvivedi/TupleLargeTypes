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
	union
	{
		dstring jsonb_string;
		materialized_numeric jsonb_numeric;
		bst jsonb_object;
		arraylist jsonb_array;
	};
};

typedef struct jsonb_object_entry jsonb_object_entry;
struct jsonb_object_entry
{
	dstring key;
	jsonb_node* value;

	bstnode jsonb_object_node;
};

// a JSONB_NULL node itself may be a NULL pointer
extern jsonb_node jsonb_true;
extern jsonb_node jsonb_false;

// below 2 functions clone internally
jsonb_node* get_jsonb_string_node(const dstring* str);
jsonb_node* get_jsonb_numeric_node(const materialized_numeric* m);

jsonb_node* get_jsonb_array_node(uint32_t capacity);
jsonb_node* get_jsonb_object_node();

#endif