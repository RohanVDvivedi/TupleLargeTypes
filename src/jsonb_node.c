#include<tuplelargetypes/jsonb_node.h>

#include<stdlib.h>

jsonb_node jsonb_true = (jsonb_node){.type = JSONB_TRUE};
jsonb_node jsonb_false = (jsonb_node){.type = JSONB_FALSE};

jsonb_node* get_jsonb_string_node(const dstring* str)
{
	jsonb_node* node_p = malloc(sizeof(jsonb_node));
	node_p->type = JSONB_STRING;

	cy_uint size = get_char_count_dstring(str);
	if(size > UINT32_MAX)
		return NULL;
	node_p->size = size;

	if(!init_copy_dstring(&(node_p->jsonb_string), str))
		exit(-1);
	return node_p;
}

jsonb_node* get_jsonb_numeric_node(const materialized_numeric* m)
{
	jsonb_node* node_p = malloc(sizeof(jsonb_node));
	node_p->type = JSONB_NUMERIC;

	cy_uint size = 3 + get_digits_count_for_materialized_numeric(m) * 5;
	if(size > UINT32_MAX)
		return NULL;
	node_p->size = size;

	if(!initialize_from_materialized_numeric(&(node_p->jsonb_numeric), NULL, 0, m))
		exit(-1);
	return node_p;
}

jsonb_node* get_jsonb_array_node(uint32_t capacity)
{
	jsonb_node* node_p = malloc(sizeof(jsonb_node));
	node_p->type = JSONB_ARRAY;
	node_p->size = 4; // 4 bytes to store element_count
	if(!initialize_arraylist(&(node_p->jsonb_array), capacity))
		exit(-1);
	return node_p;
}

static int compare_jsonb_object_entries(const void* e1, const void* e2)
{
	return compare_dstring(&(((const jsonb_object_entry*)e1)->key), &(((const jsonb_object_entry*)e2)->key));
}

jsonb_node* get_jsonb_object_node()
{
	jsonb_node* node_p = malloc(sizeof(jsonb_node));
	node_p->type = JSONB_OBJECT;
	node_p->size = 4; // 4 bytes to store element_count
	node_p->element_count = 0;
	initialize_bst(&(node_p->jsonb_object), RED_BLACK_TREE, &simple_comparator(compare_jsonb_object_entries), offsetof(jsonb_object_entry, jsonb_object_node));
	return node_p;
}