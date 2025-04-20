#include<stdio.h>
#include<stdlib.h>

#include<tuplestore/tuple.h>
#include<tuplestore/tuple_def.h>

#include<tupleindexer/interface/unWALed_in_memory_data_store.h>
#include<tupleindexer/interface/unWALed_page_modification_methods.h>

#include<tuplelargetypes/jsonb_extended.h>

//#define USE_BASE
#define USE_NESTED

#ifdef USE_BASE
	#define ACCS SELF
#else
	#define ACCS STATIC_POSITION(0)
#endif
// use ACCS as accessor to access the attribute

#define PREFIX_SIZE 15

// attributes of the page_access_specs suggestions for creating page_access_methods
#define PAGE_ID_WIDTH        3
#define PAGE_SIZE          256

// initialize transaction_id and abort_error
const void* transaction_id = NULL;
int abort_error = 0;

tuple_def tpl_d;
data_type_info* large_dti = NULL;
char tuple_type_info_memory[sizeof_tuple_data_type_info(2)];
data_type_info* tuple_dti = (data_type_info*)tuple_type_info_memory;
tuple_def* get_tuple_definition(const page_access_specs* pas_p)
{
	uint32_t max_size = PREFIX_SIZE + 10;
	large_dti = get_jsonb_extended_type_info(max_size, max_size, pas_p);

	initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 1);
	strcpy(tuple_dti->containees[0].field_name, "containee");
	tuple_dti->containees[0].al.type_info = large_dti;

	data_type_info* dti = NULL;
	#ifdef USE_NESTED
		dti = tuple_dti;
	#elif defined USE_BASE
		dti = large_dti;
	#endif

	initialize_tuple_def(&tpl_d, dti);
	print_tuple_def(&tpl_d);
	printf("\n");
	return &tpl_d;
}

jsonb_node* generate_test_data()
{
	jsonb_node* o = new_jsonb_object_node();
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("name"), new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("Rohan")));
	materialized_numeric age_val;
	initialize_static_materialized_numeric(&age_val, POSITIVE_NUMERIC, 10, (uint64_t[]){10,12,13}, 3);
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("age"), new_jsonb_numeric_node(&age_val));
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("pass"), &jsonb_false);
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("votes-right"), &jsonb_true);
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("nick-name"), NULL);

	jsonb_node* o2 = new_jsonb_object_node();
	put_in_jsonb_object_node(o2, &get_dstring_pointing_to_literal_cstring("what?"), new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("An extended library to build larger than page types for any datadata")));
	materialized_numeric version_val;
	initialize_static_materialized_numeric(&version_val, NEGATIVE_NUMERIC, -10, (uint64_t[]){1,0,1,5}, 4);
	put_in_jsonb_object_node(o2, &get_dstring_pointing_to_literal_cstring("version?"), new_jsonb_numeric_node(&version_val));
	put_in_jsonb_object_node(o2, &get_dstring_pointing_to_literal_cstring("why?"), new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("To make things going as per the MinTxEngine and BeeDB")));
	put_in_jsonb_object_node(o2, &get_dstring_pointing_to_literal_cstring("built"), &jsonb_false);
	put_in_jsonb_object_node(o2, &get_dstring_pointing_to_literal_cstring("working"), &jsonb_true);
	put_in_jsonb_object_node(o2, &get_dstring_pointing_to_literal_cstring("project-name"), new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("TupleLargeTypes")));

	jsonb_node* a = new_jsonb_array_node(10);
	push_in_jsonb_array_node(a, new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("TupleLargeTypes")));
	push_in_jsonb_array_node(a, new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("TupleIndexer")));
	push_in_jsonb_array_node(a, new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("TupleStore")));
	push_in_jsonb_array_node(a, o2);
	push_in_jsonb_array_node(a, new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("Cutlery")));
	push_in_jsonb_array_node(a, new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("MinTxEngine")));
	push_in_jsonb_array_node(a, new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("VolatilePageStore")));
	push_in_jsonb_array_node(a, new_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("BeeDB")));

	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("projects"), a);

	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("name"), NULL); // this insertion should fail jsonb only allows unique keys in objects

	return o;
}

void serialize_in_to_tuple_column(tuple_def* tpl_d, char* inline_tuple, const jsonb_node* node_p, worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	printf("INLINE TUPLE (before init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));

	binary_write_iterator* bwi_p = get_new_binary_write_iterator(inline_tuple, tpl_d, ACCS, PREFIX_SIZE, wtd_p, pam_p, pmm_p);

	printf("INLINE TUPLE (after init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));

	stream strm;
	initialize_stream_for_binary_write_iterator_static(&strm, bwi_p, transaction_id, &abort_error, 100); // write in chunks of 100 bytes at once to worm/binary_write_iterator

	if(!serialize_jsonb(&strm, node_p))
	{
		printf("error serializing jsonb node -> abort_error = %d\n", abort_error);
		exit(-1);
	}

	int error = 0;
	flush_all_from_stream(&strm, &error);
	if(error)
	{
		printf("error flushing bwi stream -> error = %d\n", error);
		exit(-1);
	}
	close_stream(&strm, &error);
	if(error)
	{
		printf("error closing bwi stream -> error = %d\n", error);
		exit(-1);
	}
	deinitialize_stream(&strm);

	delete_binary_write_iterator(bwi_p, transaction_id, &abort_error);
}

jsonb_node* parse_from_tuple_column(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p)
{
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));
	printf("hash => %"PRIu64"\n\n", hash_blob(tpl_d, inline_tuple, ACCS, FNV_64_TUPLE_HASHER, wtd_p, pam_p, transaction_id, &abort_error));

	binary_read_iterator* bri_p = get_new_binary_read_iterator(inline_tuple, tpl_d, ACCS, wtd_p, pam_p);

	stream strm;
	initialize_stream_for_binary_read_iterator_static(&strm, bri_p, transaction_id, &abort_error);

	jsonb_node* node_p = parse_jsonb(&strm);
	if(node_p == NULL)
	{
		printf("jsonb parser errored -> abort_error = %d\n", abort_error);
		exit(-1);
	}

	int error = 0;
	close_stream(&strm, &error);
	if(error)
	{
		printf("error closing bwi stream -> error = %d\n", error);
		exit(-1);
	}
	deinitialize_stream(&strm);

	delete_binary_read_iterator(bri_p, transaction_id, &abort_error);

	return node_p;
}

void print_json_attribute(const jsonb_node* node_p, json_accessor acs)
{
	int non_existing = 0;
	const jsonb_node* attr = fetch_jsonb_from_jsonb(node_p, acs, &non_existing);
	printf("jsonb");
	for(uint64_t i = 0; i < acs.keys_length; i++)
	{
		printf("[");
		if(acs.keys_list[i].is_array_index)
			printf("%"PRIu_cy_uint, acs.keys_list[i].index);
		else
			printf("\""printf_dstring_format"\"", printf_dstring_params(&(acs.keys_list[i].key)));
		printf("]");
	}
	printf("\n");
	if(non_existing)
		printf("non_existing");
	else
		print_jsonb(attr, 0);
	printf("\n\n");
}

void print_worm_as_is(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p)
{
	uint64_t head_page_id = get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas));
	print_worm(head_page_id, wtd_p, pam_p, transaction_id, &abort_error);
	if(abort_error)
	{
		printf("abort error printing worm\n");
		exit(-1);
	}
}

int main()
{
	// construct an in-memory data store
	page_access_methods* pam_p = get_new_unWALed_in_memory_data_store(&((page_access_specs){.page_id_width = PAGE_ID_WIDTH, .page_size = PAGE_SIZE}));

	// construct unWALed page_modification_methods
	page_modification_methods* pmm_p = get_new_unWALed_page_modification_methods();

	// construct tuple definitions for worm
	worm_tuple_defs wtd;
	init_worm_tuple_definitions(&wtd, &(pam_p->pas));

	// allocate record tuple definition and initialize it
	tuple_def* tpl_d = get_tuple_definition(&(pam_p->pas));
	{
		const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, ACCS);
		printf("is_inline = %d, is_extended = %d\n", is_inline_type_info(dti_p), is_extended_type_info(dti_p));
	}

	/* TESTS STARTED */

	char inline_tuple[PAGE_SIZE];
	init_tuple(tpl_d, inline_tuple);
	#ifdef USE_NESTED
		set_element_in_tuple(tpl_d, ACCS, inline_tuple, EMPTY_USER_VALUE, UINT32_MAX);
	#endif

	// generate test jsonb
	jsonb_node* n1_p = generate_test_data();

	uint32_t total_size = 0;
	int finalized = finalize_jsonb(n1_p, &total_size);
	printf("finalized = %d, total_size %"PRIu32"\n\n", finalized, total_size);
	print_jsonb(n1_p, 0);printf("\n\n");
	printf("----------------------------------------------------\n\n");

	serialize_in_to_tuple_column(tpl_d, inline_tuple, n1_p, &wtd, pam_p, pmm_p);

	print_worm_as_is(tpl_d, inline_tuple, &wtd, pam_p);

	jsonb_node* n2_p = parse_from_tuple_column(tpl_d, inline_tuple, &wtd, pam_p);

	finalized = finalize_jsonb(n2_p, &total_size);
	printf("finalized = %d, total_size %"PRIu32"\n\n", finalized, total_size);
	print_jsonb(n2_p, 0);printf("\n\n");
	printf("----------------------------------------------------\n\n");

	printf("is node serialized same as that is parsed = %d\n\n", are_equal_jsonb(n1_p, n2_p));

	jsonb_node* n3_p = clone_jsonb(n1_p);

	finalized = finalize_jsonb(n3_p, &total_size);
	printf("finalized = %d, total_size %"PRIu32"\n\n", finalized, total_size);
	print_jsonb(n3_p, 0);printf("\n\n");
	printf("----------------------------------------------------\n\n");

	printf("is node after clone the same = %d\n\n", are_equal_jsonb(n1_p, n3_p));

	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR());
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("votes-right")));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("nick-name")));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("nick-names")));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("projects")));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("projects"), JSON_ARRAY_INDEX(2)));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("projects"), JSON_ARRAY_INDEX(10)));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("projects"), JSON_ARRAY_INDEX(3)));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("projects"), JSON_ARRAY_INDEX(3), JSON_OBJECT_KEY_literal("version?")));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("projects"), JSON_ARRAY_INDEX(3), JSON_OBJECT_KEY_literal("project-name")));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("projects"), JSON_ARRAY_INDEX(3), JSON_OBJECT_KEY_literal("project-names")));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("projects"), JSON_ARRAY_INDEX(3), JSON_OBJECT_KEY_literal("project-name"), JSON_ARRAY_INDEX(0)));
	print_json_attribute(n1_p, STATIC_JSON_ACCESSOR(JSON_OBJECT_KEY_literal("projects"), JSON_ARRAY_INDEX(3), JSON_OBJECT_KEY_literal("project-name"), JSON_OBJECT_KEY_literal("none")));

	delete_jsonb_node(n3_p);
	delete_jsonb_node(n2_p);
	delete_jsonb_node(n1_p);

	/* TESTS ENDED */

	/* CLEANUP */

	// destroy worm
	uint64_t head_page_id = get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas));
	uint64_t dependent_root_page_id;
	int vaccum_needed = 0;
	if(head_page_id != pam_p->pas.NULL_PAGE_ID)
	{
		decrement_reference_counter_for_worm(head_page_id, &dependent_root_page_id, &vaccum_needed, &wtd, pam_p, pmm_p, transaction_id, &abort_error);
		printf("dependent_root_page_id = %"PRIu64" vaccum_needed = %d\n", dependent_root_page_id, vaccum_needed);
	}

	// close the in-memory data store
	close_and_destroy_unWALed_in_memory_data_store(pam_p);

	// destroy worm_tuple_definitions
	deinit_worm_tuple_definitions(&wtd);

	// destory page_modification_methods
	delete_unWALed_page_modification_methods(pmm_p);

	free(large_dti->containees[0].al.type_info);
	free(large_dti);
	return 0;
}