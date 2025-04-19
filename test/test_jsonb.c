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
	jsonb_node* o = get_jsonb_object_node();
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("name"), get_jsonb_string_node(&get_dstring_pointing_to_literal_cstring("Rohan")));
	materialized_numeric age_val;
	initialize_static_materialized_numeric(&age_val, POSITIVE_NUMERIC, 10, (uint64_t[]){10,12,13}, 3);
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("age"), get_jsonb_numeric_node(&age_val));
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("pass"), &jsonb_false);
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("votes-right"), &jsonb_true);
	put_in_jsonb_object_node(o, &get_dstring_pointing_to_literal_cstring("nick-name"), NULL);

	return o;
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