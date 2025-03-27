#include<stdio.h>
#include<stdlib.h>

#include<tuple.h>
#include<tuple_def.h>

#include<unWALed_in_memory_data_store.h>
#include<unWALed_page_modification_methods.h>

#include<blob_extended.h>
#include<text_extended.h>

//#define USE_INLINE
#define USE_EXTENDED

//#define USE_BASE
#define USE_NESTED

#ifdef USE_BASE
	#define ACCS SELF
#else
	#define ACCS STATIC_POSITION(0)
#endif
// use ACCS as accessor to access the attribute

#define PREFIX_SIZE 15

#define READ_CHUNK_SIZE 5
//#define READ_CHUNK_SIZE 100

#define WRITE_CHUNK_SIZE 5
//#define WRITE_CHUNK_SIZE 100

// attributes of the page_access_specs suggestions for creating page_access_methods
#define PAGE_ID_WIDTH        3
#define PAGE_SIZE          256

// initialize transaction_id and abort_error
const void* transaction_id = NULL;
int abort_error = 0;

char* test_data = "Rohan is a good boy, "
					"Rohan is probably a bad boy, "
					"Common sense is not all that common, "
					"Does Rohan have any? "
					"Not if this code functions as required, "
					"What else can I write here? "
					"No one cares what would be written here. "
					"I will probably change it to lorem ipsum once this project goes big enough.";

tuple_def tpl_d;
data_type_info* short_dti = NULL;
data_type_info* large_dti = NULL;
char tuple_type_info_memory[sizeof_tuple_data_type_info(2)];
data_type_info* tuple_dti = (data_type_info*)tuple_type_info_memory;
tuple_def* get_tuple_definition(const page_access_specs* pas_p)
{
	uint32_t max_size = PREFIX_SIZE + 10;
	short_dti = get_text_inline_type_info(max_size);
	large_dti = get_text_extended_type_info(max_size, short_dti, pas_p);

	initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 1);
	strcpy(tuple_dti->containees[0].field_name, "containee");
	#ifdef USE_INLINE
		tuple_dti->containees[0].al.type_info = short_dti;
	#else
		tuple_dti->containees[0].al.type_info = large_dti;
	#endif

	data_type_info* dti = NULL;
	#ifdef USE_NESTED
		dti = tuple_dti;
	#elif defined USE_BASE
		#ifdef USE_INLINE
			dti = short_dti;
		#elif defined USE_EXTENDED
			dti = large_dti;
		#endif
	#endif

	initialize_tuple_def(&tpl_d, dti);
	print_tuple_def(&tpl_d);
	printf("\n");
	return &tpl_d;
}

void insert_all_test_data(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	printf("INLINE TUPLE (before init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));

	binary_write_iterator* tbwi_p = get_new_binary_write_iterator(inline_tuple, tpl_d, ACCS, PREFIX_SIZE, wtd_p, pam_p, pmm_p);

	printf("INLINE TUPLE (after init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));

	const uint32_t TEST_DATA_SIZE = strlen(test_data);

	char* bytes = test_data;
	uint32_t bytes_to_write = TEST_DATA_SIZE;
	uint32_t bytes_written = 0;
	while(bytes_to_write > 0)
	{
		uint32_t bytes_to_write_this_iteration = min(bytes_to_write, WRITE_CHUNK_SIZE);

		bytes_to_write_this_iteration = append_to_binary_write_iterator(tbwi_p, bytes, bytes_to_write_this_iteration, transaction_id, &abort_error);

		if(bytes_to_write_this_iteration == 0)
			break;

		printf("bytes_written_this_iteration = %"PRIu32"\n", bytes_to_write_this_iteration);

		bytes += bytes_to_write_this_iteration;
		bytes_to_write -= bytes_to_write_this_iteration;
		bytes_written += bytes_to_write_this_iteration;
	}

	printf("bytes_written = %"PRIu32"/%"PRIu32"\n\n", bytes_written, TEST_DATA_SIZE);

	delete_binary_write_iterator(tbwi_p, transaction_id, &abort_error);
}

void read_and_compare_all_test_data(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p)
{
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));
	printf("hash => %"PRIu64"\n\n", hash_blob(tpl_d, inline_tuple, ACCS, FNV_64_TUPLE_HASHER, wtd_p, pam_p, transaction_id, &abort_error));

	binary_read_iterator* tbri_p = get_new_binary_read_iterator(inline_tuple, tpl_d, ACCS, wtd_p, pam_p);

	const uint32_t TEST_DATA_SIZE = strlen(test_data);

	char read_buffer[READ_CHUNK_SIZE];
	uint32_t bytes_read = 0;
	while(1)
	{
		uint32_t bytes_read_this_iteration = read_from_binary_read_iterator(tbri_p, read_buffer, READ_CHUNK_SIZE, transaction_id, &abort_error);
		if(bytes_read_this_iteration == 0)
			break;

		int matches = 1;
		for(uint32_t i = 0; i < bytes_read_this_iteration && matches == 1; i++)
			matches = (read_buffer[i] == test_data[(bytes_read + i) % TEST_DATA_SIZE]);

		printf(" ->\"%.*s\" matches => %d\n", bytes_read_this_iteration, read_buffer, matches);

		bytes_read += bytes_read_this_iteration;
	}

	printf("bytes_read = %"PRIu32"/%"PRIu32"\n\n", bytes_read, TEST_DATA_SIZE);

	delete_binary_read_iterator(tbri_p, transaction_id, &abort_error);
}

void compare_tests();

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
	read_and_compare_all_test_data(tpl_d, inline_tuple, &wtd, pam_p);

	insert_all_test_data(tpl_d, inline_tuple, &wtd, pam_p, pmm_p);
	read_and_compare_all_test_data(tpl_d, inline_tuple, &wtd, pam_p);

	insert_all_test_data(tpl_d, inline_tuple, &wtd, pam_p, pmm_p);
	read_and_compare_all_test_data(tpl_d, inline_tuple, &wtd, pam_p);

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

	// run comparison based tests
	compare_tests(&wtd, pam_p, pmm_p);

	// close the in-memory data store
	close_and_destroy_unWALed_in_memory_data_store(pam_p);

	// destroy worm_tuple_definitions
	deinit_worm_tuple_definitions(&wtd);

	// destory page_modification_methods
	delete_unWALed_page_modification_methods(pmm_p);

	// delete the record definition
	free(short_dti);
	free(large_dti);

	return 0;
}

void set_and_compare(char* s1, char* s2, char* tuple, const tuple_def* tpl_d, worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	init_tuple(tpl_d, tuple);

	// set s1 in tuple
	if(s1 != NULL)
	{
		binary_write_iterator* tbwi_p = get_new_binary_write_iterator(tuple, tpl_d, STATIC_POSITION(0), PREFIX_SIZE, wtd_p, pam_p, pmm_p);

		const uint32_t TEST_DATA_SIZE = strlen(test_data);

		char* bytes = s1;
		uint32_t bytes_to_write = strlen(s1);
		uint32_t bytes_written = 0;
		while(bytes_to_write > 0)
		{
			uint32_t bytes_to_write_this_iteration = bytes_to_write;
			bytes_to_write_this_iteration = append_to_binary_write_iterator(tbwi_p, bytes, bytes_to_write_this_iteration, transaction_id, &abort_error);

			if(bytes_to_write_this_iteration == 0)
				break;

			bytes += bytes_to_write_this_iteration;
			bytes_to_write -= bytes_to_write_this_iteration;
			bytes_written += bytes_to_write_this_iteration;
		}

		delete_binary_write_iterator(tbwi_p, transaction_id, &abort_error);
	}

	// set s2 in tuple
	if(s2 != NULL)
	{
		binary_write_iterator* tbwi_p = get_new_binary_write_iterator(tuple, tpl_d, STATIC_POSITION(1), PREFIX_SIZE, wtd_p, pam_p, pmm_p);

		const uint32_t TEST_DATA_SIZE = strlen(test_data);

		char* bytes = s2;
		uint32_t bytes_to_write = strlen(s2);
		uint32_t bytes_written = 0;
		while(bytes_to_write > 0)
		{
			uint32_t bytes_to_write_this_iteration = bytes_to_write;
			bytes_to_write_this_iteration = append_to_binary_write_iterator(tbwi_p, bytes, bytes_to_write_this_iteration, transaction_id, &abort_error);

			if(bytes_to_write_this_iteration == 0)
				break;

			bytes += bytes_to_write_this_iteration;
			bytes_to_write -= bytes_to_write_this_iteration;
			bytes_written += bytes_to_write_this_iteration;
		}

		delete_binary_write_iterator(tbwi_p, transaction_id, &abort_error);
	}

	{
		int cmp = 100;
		int prefix = 100;
		cmp = compare_tb(tpl_d, tuple, STATIC_POSITION(0), wtd_p, pam_p, transaction_id, &abort_error,
				tpl_d, tuple, STATIC_POSITION(1), wtd_p, pam_p, transaction_id, &abort_error,
				&prefix);
		printf("%s, %s => cmp(%d), prefix(%d)\n", s1, s2, cmp, prefix);
	}

	{
		int cmp = 100;
		int prefix = 100;
		cmp = compare_tb(tpl_d, tuple, STATIC_POSITION(1), wtd_p, pam_p, transaction_id, &abort_error,
				tpl_d, tuple, STATIC_POSITION(0), wtd_p, pam_p, transaction_id, &abort_error,
				&prefix);
		printf("%s, %s => cmp(%d), prefix(%d)\n", s2, s1, cmp, prefix);
	}
	printf("\n");

	// destroy s1
	{
		uint64_t head_page_id = get_extension_head_page_id_for_extended_type(tuple, tpl_d, STATIC_POSITION(0), &(pam_p->pas));
		uint64_t dependent_root_page_id;
		int vaccum_needed = 0;
		if(head_page_id != pam_p->pas.NULL_PAGE_ID)
			decrement_reference_counter_for_worm(head_page_id, &dependent_root_page_id, &vaccum_needed, wtd_p, pam_p, pmm_p, transaction_id, &abort_error);
	}

	// destroy s2
	{
		uint64_t head_page_id = get_extension_head_page_id_for_extended_type(tuple, tpl_d, STATIC_POSITION(1), &(pam_p->pas));
		uint64_t dependent_root_page_id;
		int vaccum_needed = 0;
		if(head_page_id != pam_p->pas.NULL_PAGE_ID)
			decrement_reference_counter_for_worm(head_page_id, &dependent_root_page_id, &vaccum_needed, wtd_p, pam_p, pmm_p, transaction_id, &abort_error);
	}
}

void compare_tests(worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	char tuple[1024];

	char tuple_type_info_memory[sizeof_tuple_data_type_info(2)];
	data_type_info* tuple_dti = (data_type_info*)tuple_type_info_memory;
	initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 2);
	strcpy(tuple_dti->containees[0].field_name, "containee1");
	strcpy(tuple_dti->containees[1].field_name, "containee2");

	{
		tuple_dti->containees[0].al.type_info = short_dti;
		tuple_dti->containees[1].al.type_info = short_dti;
		initialize_tuple_def(&tpl_d, tuple_dti);

		set_and_compare(NULL, NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(NULL, "abc", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("def", NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abc", "abc", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abc", "def", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abcd", "abce", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abc", "abcd", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
	}

	{
		tuple_dti->containees[0].al.type_info = large_dti;
		tuple_dti->containees[1].al.type_info = short_dti;
		initialize_tuple_def(&tpl_d, tuple_dti);

		set_and_compare(NULL, NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(NULL, "abc", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("def", NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abc", "abc", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abc", "def", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abcd", "abce", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abc", "abcd", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
	}

	{
		tuple_dti->containees[0].al.type_info = large_dti;
		tuple_dti->containees[1].al.type_info = large_dti;
		initialize_tuple_def(&tpl_d, tuple_dti);

		set_and_compare(NULL, NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(NULL, "abc", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("def", NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abc", "abc", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abc", "def", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abcd", "abce", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare("abc", "abcd", tuple, &tpl_d, wtd_p, pam_p, pmm_p);
	}
}