#include<tuplelargetypes/jsonb_node.h>

#include<stdlib.h>

jsonb_node jsonb_true = (jsonb_node){.type = JSONB_TRUE};
jsonb_node jsonb_false = (jsonb_node){.type = JSONB_FALSE};

jsonb_node* get_jsonb_string_node(const dstring* str)
{
	jsonb_node* node_p = malloc(sizeof(jsonb_node));
	node_p->type = JSONB_STRING;
	if(!init_copy_dstring(&(node_p->string_value), str))
		exit(-1);
	return node_p;
}

jsonb_node* get_jsonb_numeric_node(const materialized_numeric* m)
{
	jsonb_node* node_p = malloc(sizeof(jsonb_node));
	node_p->type = JSONB_NUMERIC;
	if(!initialize_from_materialized_numeric(&(node_p->numeric_value), NULL, 0, m))
		exit(-1);
	return node_p;
}