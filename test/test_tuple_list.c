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
	initialize_tuple_data_type_info(tpl_d_tlist_elements_dti, "elments", 1, PAGE_SIZE, 2);
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
	uint32_t max_size = PREFIX_SIZE + 10;
	tlist_dti = get_tuple_list_extended_type_info(max_size, PREFIX_SIZE + 10, pas_p);

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

uint32_t build_tuple(void* res, const char* sval, double dval)
{
	init_tuple(&tpl_d_tlist_elements, res);
	set_element_in_tuple(&tpl_d_tlist_elements, STATIC_POSITION(0), res, &(user_value){.string_value = sval, .string_size = strlen(sval)}, UINT32_MAX);
	set_element_in_tuple(&tpl_d_tlist_elements, STATIC_POSITION(1), res, &(user_value){.double_value = dval}, UINT32_MAX);
	return get_tuple_size(&tpl_d_tlist_elements, res);
}

void insert_test_tuples(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p, double* dvals, char const * const * svals)
{
	printf("INLINE TUPLE (before init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));

	binary_write_iterator* tbwi_p = get_new_binary_write_iterator(inline_tuple, tpl_d, ACCS, PREFIX_SIZE, wtd_p, pam_p, pmm_p);

	printf("INLINE TUPLE (after init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));

	while((*dvals) != 0.0)
	{
		char tuple[1024];
		uint32_t bytes_to_write_this_iteration = build_tuple(tuple, *svals, *dvals);

		bytes_to_write_this_iteration = append_to_binary_write_iterator(tbwi_p, tuple, bytes_to_write_this_iteration, transaction_id, &abort_error);

		if(bytes_to_write_this_iteration == 0)
			break;

		printf("written-> ");
		print_tuple(tuple, &tpl_d_tlist_elements);
		printf("bytes_written_this_iteration = %"PRIu32"\n", bytes_to_write_this_iteration);

		dvals++;
		svals++;
	}

	delete_binary_write_iterator(tbwi_p, transaction_id, &abort_error);
	printf("\n\n");
}

void read_and_skip_test_tuples(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p, int const * read_or_skip)
{
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));

	binary_read_iterator* tbri_p = get_new_binary_read_iterator(inline_tuple, tpl_d, ACCS, wtd_p, pam_p);

	while((*read_or_skip) != -1)
	{
		if(*read_or_skip)
		{
			void* tuple = read_tuple_from_binary_read_iterator(tbri_p, &tpl_d_tlist_elements, transaction_id, &abort_error);
			if(tuple)
			{
				printf("read-> ");
				print_tuple(tuple, &tpl_d_tlist_elements);
				free(tuple);
			}
			else
			{
				printf("read failed\n");
				break;
			}
		}
		else
		{
			int skipped = skip_tuple_from_binary_read_iterator(tbri_p, &tpl_d_tlist_elements, transaction_id, &abort_error);
			if(skipped)
				printf("skipped\n");
			else
			{
				printf("skip failed\n");
				break;
			}
		}
		read_or_skip++;
	}

	delete_binary_read_iterator(tbri_p, transaction_id, &abort_error);
	printf("\n\n");
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
	read_and_skip_test_tuples(tpl_d, inline_tuple, &wtd, pam_p, (int[]){1,0,1,0,-1});
	read_and_skip_test_tuples(tpl_d, inline_tuple, &wtd, pam_p, (int[]){0,1,0,1,-1});

	insert_test_tuples(tpl_d, inline_tuple, &wtd, pam_p, pmm_p, (double[]){1.0,2.0,4.0,8.0,0.0}, (char const*[]){"This is one - Rohan","This is two - Rupa","This is four - Devashree","This is eight - Vipulkumar"});
	read_and_skip_test_tuples(tpl_d, inline_tuple, &wtd, pam_p, (int[]){1,0,1,0,-1});
	read_and_skip_test_tuples(tpl_d, inline_tuple, &wtd, pam_p, (int[]){0,1,0,1,-1});

	insert_test_tuples(tpl_d, inline_tuple, &wtd, pam_p, pmm_p, (double[]){16.0,32.0,64.0,128.0,0.0}, (char const*[]){"This is sixteen - Rohan Vipulkumar Dvivedi","This is thirty two - Rupa Vipulkumar Dvivedi","This is sixty four - Devashree Manan Joshi","This is one twenty eight - Vipulkumar Bhanuprasad Dvivedi"});
	read_and_skip_test_tuples(tpl_d, inline_tuple, &wtd, pam_p, (int[]){1,0,0,1,1,0,1,0,-1});
	read_and_skip_test_tuples(tpl_d, inline_tuple, &wtd, pam_p, (int[]){0,1,1,0,0,1,0,1,-1});

	read_and_skip_test_tuples(tpl_d, inline_tuple, &wtd, pam_p, (int[]){1,1,1,1,1,1,1,1,1,-1});

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