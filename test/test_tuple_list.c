#include<stdio.h>
#include<stdlib.h>

#include<tuple.h>
#include<tuple_def.h>

#include<unWALed_in_memory_data_store.h>
#include<unWALed_page_modification_methods.h>

#include<tuple_list_extended.h>

//#define USE_BASE
#define USE_NESTED

//#define FIXED_LENGTH
#define VARIABLE_LENGTH

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
char tuple_type_info_memory[sizeof_tuple_data_type_info(2)];
data_type_info* tuple_dti = (data_type_info*)tuple_type_info_memory;
data_type_info* tlist_dti;

tuple_def tpl_d_tlist_elements;
char tuple_type_info_memory2[sizeof_tuple_data_type_info(2)];
data_type_info* tpl_d_tlist_elements_dti = (data_type_info*)tuple_type_info_memory2;
data_type_info string_0;

void init_tpl_d_tlist_elements()
{
	initialize_tuple_data_type_info(tpl_d_tlist_elements_dti, "elments", 1, PAGE_SIZE, 1);
	strcpy(tpl_d_tlist_elements_dti->containees[0].field_name, "varchar");
	#ifdef FIXED_LENGTH
		string_0 = get_fixed_length_string_type("char", 64, 1);
	#elif defined VARIABLE_LENGTH
		string_0 = get_variable_length_string_type("varchar", 64);
	#endif
	tpl_d_tlist_elements_dti->containees[0].al.type_info = &string_0;
	strcpy(tpl_d_tlist_elements_dti->containees[1].field_name, "float");
	tpl_d_tlist_elements_dti->containees[1].al.type_info = FLOAT_double_NON_NULLABLE;
	initialize_tuple_def(&tpl_d_tlist_elements, tpl_d_tlist_elements_dti);
}

tuple_def* get_tuple_definition(const page_access_specs* pas_p)
{
	data_type_info* tlist_dti = get_tuple_list_extended_type_info(PREFIX_SIZE + 10, pas_p);

	initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 1);
	strcpy(tuple_dti->containees[0].field_name, "containee");
	tuple_dti->containees[0].al.type_info = tlist_dti;

	data_type_info* dti = NULL;
	#ifdef USE_NESTED
		dti = tuple_dti;
	#elif defined USE_BASE
		dti = tlist_dti;
	#endif

	initialize_tuple_def(&tpl_d, dti);
	print_tuple_def(&tpl_d);
	printf("\n\n");
	init_tpl_d_tlist_elements();
	print_tuple_def(&tpl_d_tlist_elements);
	printf("\n\n");
	return &tpl_d;
}

int main()
{
	/* SETUP STARTED */

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

	// delete the record definition
	free(tlist_dti->containees[0].al.type_info);
	free(tlist_dti);

	return 0;
}