#include<tuplelargetypes/jsonb_node.h>

#include<stdlib.h>

jsonb_node jsonb_true = (jsonb_node){.type = JSONB_TRUE};
jsonb_node jsonb_false = (jsonb_node){.type = JSONB_FALSE};

jsonb_node* get_jsonb_string_node(const dstring* str)
{
	jsonb_node* node_p = malloc(sizeof(jsonb_node));
	if(node_p == NULL)
		exit(-1);
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
	if(node_p == NULL)
		exit(-1);
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
	if(node_p == NULL)
		exit(-1);
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
	if(node_p == NULL)
		exit(-1);
	node_p->type = JSONB_OBJECT;
	node_p->size = 4; // 4 bytes to store element_count
	node_p->element_count = 0;
	initialize_bst(&(node_p->jsonb_object), RED_BLACK_TREE, &simple_comparator(compare_jsonb_object_entries), offsetof(jsonb_object_entry, jsonb_object_node));
	return node_p;
}

static void notify_and_delete_jsonb_object_entry(void* resource_p, const void* data_p)
{
	jsonb_object_entry* e = (jsonb_object_entry*) data_p;
	deinit_dstring(&(e->key));
	delete_jsonb_node(e->value);
	free(e);
}

int push_in_jsonb_array_node(jsonb_node* array_p, jsonb_node* node_p)
{
	if(array_p == NULL || array_p->type != JSONB_ARRAY)
		return 0;

	// TODO check size

	// TODO manage size

	if(is_full_arraylist(&(array_p->jsonb_array)) && !expand_arraylist(&(array_p->jsonb_array))) // if full and the expand fails
		exit(-1);
	return push_back_to_arraylist(&(array_p->jsonb_array), node_p);
}

int put_in_jsonb_object_node(jsonb_node* object_p, const dstring* key, jsonb_node* node_p)
{
	if(object_p == NULL || object_p->type != JSONB_OBJECT)
		return 0;

	// TODO check size

	// TODO manage size

	// TODO
}

void delete_jsonb_node(jsonb_node* node_p)
{
	if(node_p == NULL)
		return;

	switch(node_p->type)
	{
		case JSONB_NULL :
			break;
		case JSONB_TRUE :
		case JSONB_FALSE :
		{
			if(node_p == &jsonb_true || node_p == &jsonb_false)
				return;
			break;
		}
		case JSONB_STRING :
		{
			deinit_dstring(&(node_p->jsonb_string));
			break;
		}
		case JSONB_NUMERIC :
		{
			deinitialize_materialized_numeric(&(node_p->jsonb_numeric));
			break;
		}
		case JSONB_ARRAY :
		{
			while(!is_empty_arraylist(&(node_p->jsonb_array)))
			{
				jsonb_node* n = (jsonb_node*) get_front_of_arraylist(&(node_p->jsonb_array));
				pop_front_from_arraylist(&(node_p->jsonb_array));
				delete_jsonb_node(n);
			}
			deinitialize_arraylist(&(node_p->jsonb_array));
			break;
		}
		case JSONB_OBJECT :
		{
			remove_all_from_bst(&(node_p->jsonb_object), &((notifier_interface){NULL, notify_and_delete_jsonb_object_entry}));
			break;
		}
	}

	free(node_p);
}