#include<stdio.h>
#include<stdlib.h>

#include<tuplestore/tuple.h>
#include<tuplestore/tuple_def.h>

#include<tupleindexer/interface/unWALed_in_memory_data_store.h>
#include<tupleindexer/interface/unWALed_page_modification_methods.h>

#include<tuplelargetypes/blob_extended.h>
#include<tuplelargetypes/text_extended.h>

#include<tupleindexer/heap_table/heap_table.h>
#include<tupleindexer/blob_store/blob_store.h>

#include<tupleindexer/utils/heap_table_accumulative_notifier.h>

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

heap_table_accumulative_notifier htan;

void fix_all_entries(const heap_table_tuple_defs* httd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p)
{
	uint64_t root_page_id;
	uint32_t unused_space;
	uint64_t page_id;
	while(pop_from_heap_table_accumulative_notifier(&htan, &root_page_id, &unused_space, &page_id))
	{
		fix_unused_space_in_heap_table(root_page_id, unused_space, page_id, httd_p, pam_p, pmm_p, transaction_id, &abort_error);
		if(abort_error)
		{
			printf("ABORTED\n");
			exit(-1);
		}
	}
}

uint64_t blob_store_root_page_id;
tuple_pointer extension_tail;

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
	large_dti = get_text_extended_type_info("my_type", max_size, short_dti, pas_p);

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
	printf("%d\n\n", has_extended_type_info(tpl_d.type_info, "my_type"));
	uint32_t sub_type_len = 0;
	const char* sub_type = get_extension_sub_type_for_extended_type(get_type_info_for_element_from_tuple_def(&tpl_d, ACCS), &sub_type_len);
	if(sub_type != NULL)
		printf("sub_type = <%.*s>\n\n", sub_type_len, sub_type);
	return &tpl_d;
}

void insert_all_test_data(tuple_def* tpl_d, char* inline_tuple, const blob_store_tuple_defs* bstd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	datum uval;
	const data_type_info* dti = get_type_info_for_element_from_tuple_def(tpl_d, ACCS);
	get_value_from_element_from_tuple(&uval, tpl_d, ACCS, inline_tuple);

	printf("INLINE TUPLE (before init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);

	binary_write_iterator* tbwi_p = get_new_binary_write_iterator(inline_tuple, tpl_d, ACCS, blob_store_root_page_id, extension_tail, PREFIX_SIZE, bstd_p, pam_p, pmm_p);

	dti = get_type_info_for_element_from_tuple_def(tpl_d, ACCS);
	get_value_from_element_from_tuple(&uval, tpl_d, ACCS, inline_tuple);

	printf("INLINE TUPLE (after init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);

	const uint32_t TEST_DATA_SIZE = strlen(test_data);

	char* bytes = test_data;
	uint32_t bytes_to_write = TEST_DATA_SIZE;
	uint32_t bytes_written = 0;
	while(bytes_to_write > 0)
	{
		uint32_t bytes_to_write_this_iteration = min(bytes_to_write, WRITE_CHUNK_SIZE);

		bytes_to_write_this_iteration = append_to_binary_write_iterator(tbwi_p, bytes, bytes_to_write_this_iteration, &HEAP_TABLE_ACCUMULATIVE_NOTIFIER(&htan), transaction_id, &abort_error);

		fix_all_entries(&(bstd_p->httd), pam_p, pmm_p);

		if(bytes_to_write_this_iteration == 0)
			break;

		printf("bytes_written_this_iteration = %"PRIu32"\n", bytes_to_write_this_iteration);

		bytes += bytes_to_write_this_iteration;
		bytes_to_write -= bytes_to_write_this_iteration;
		bytes_written += bytes_to_write_this_iteration;
		extension_tail = tbwi_p->extension_tail;
	}

	printf("bytes_written = %"PRIu32"/%"PRIu32"\n\n", bytes_written, TEST_DATA_SIZE);

	delete_binary_write_iterator(tbwi_p, transaction_id, &abort_error);
}

void read_and_compare_all_test_data(tuple_def* tpl_d, char* inline_tuple, blob_store_tuple_defs* bstd_p, page_access_methods* pam_p)
{
	datum uval;
	const data_type_info* dti = get_type_info_for_element_from_tuple_def(tpl_d, ACCS);
	get_value_from_element_from_tuple(&uval, tpl_d, ACCS, inline_tuple);

	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("hash => %"PRIu64"\n\n", hash_blob(&uval, dti, FNV_64_TUPLE_HASHER, bstd_p, pam_p, transaction_id, &abort_error));

	binary_read_iterator* tbri_p = get_new_binary_read_iterator(&uval, dti, bstd_p, pam_p, NULL);

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
	{
		tuple_hasher th = *FNV_64_TUPLE_HASHER;
		printf("hash %d of test data -> %"PRIu64"\n", 0, th.hash);
		int test_data_len = strlen(test_data);
		for(int i = 0; i < 3; i++)
		{
			uint64_t h = tuple_hash_bytes(&th, test_data, test_data_len);
			printf("hash %d of test data -> %"PRIu64"\n", i+1, h);
		}
		printf("\n");
	}

	/* SETUP STARTED */

	// setup notifier
	initialize_heap_table_accumulative_notifier(&htan, 24);

	// construct an in-memory data store
	page_access_methods* pam_p = get_new_unWALed_in_memory_data_store(&((page_access_specs){.page_id_width = PAGE_ID_WIDTH, .page_size = PAGE_SIZE}));

	// construct unWALed page_modification_methods
	page_modification_methods* pmm_p = get_new_unWALed_page_modification_methods();

	// construct tuple definitions for blob_store
	blob_store_tuple_defs bstd;
	init_blob_store_tuple_definitions(&bstd, &(pam_p->pas));

	// create a blob_store
	blob_store_root_page_id = get_new_blob_store(&bstd, pam_p, pmm_p, transaction_id, &abort_error);
	if(abort_error)
	{
		printf("ABORTED\n");
		exit(-1);
	}

	// allocate record tuple definition and initialize it
	tuple_def* tpl_d = get_tuple_definition(&(pam_p->pas));
	{
		const data_type_info* dti_p = get_type_info_for_element_from_tuple_def(tpl_d, ACCS);
		printf("is_inline = %d, is_extended = %d, is_text = %d, is_blob = %d\n", is_inline_type_info(dti_p), is_extended_type_info(dti_p), is_text_type_info(dti_p), is_blob_type_info(dti_p));
	}

	/* TESTS STARTED */

	char inline_tuple[PAGE_SIZE];
	init_tuple(tpl_d, inline_tuple);
	#ifdef USE_NESTED
		set_element_in_tuple(tpl_d, ACCS, inline_tuple, EMPTY_DATUM, UINT32_MAX);
	#endif
	read_and_compare_all_test_data(tpl_d, inline_tuple, &bstd, pam_p);

	insert_all_test_data(tpl_d, inline_tuple, &bstd, pam_p, pmm_p);
	read_and_compare_all_test_data(tpl_d, inline_tuple, &bstd, pam_p);

	char* test5 = alloca((strlen(test_data) * 3) + 1);
	memory_move(test5, test_data, strlen(test_data));
	memory_move(test5 + strlen(test_data), test_data, strlen(test_data));
	memory_move(test5 + 2 * strlen(test_data), test_data, strlen(test_data));
	test5[3 * strlen(test_data)] = '\0';

	char* compare_with[] = {
		NULL,
		"",
		"Rohan",
		"Rohan is a good boy, ",
		"Rohan is a good boy,X",
		test_data,
		test5,
	};

	for(int i = 0; i < sizeof(compare_with)/sizeof(compare_with[0]); i++)
	{
		{
			datum uval;
			const data_type_info* dti = get_type_info_for_element_from_tuple_def(tpl_d, ACCS);
			get_value_from_element_from_tuple(&uval, tpl_d, ACCS, inline_tuple);
			binary_read_iterator* bri1 = get_new_binary_read_iterator(&uval, dti, &bstd, pam_p, NULL);
			binary_read_iterator* bri2 = get_new_binary_read_iterator((compare_with[i] == NULL) ? (NULL_DATUM) : &((datum){.string_or_binary_value = compare_with[i], .string_or_binary_size = strlen(compare_with[i])}), NULL, &bstd, pam_p, NULL);
			int cmp = 100;
			int prefix = 100;
			cmp = compare_tb(bri1, bri2, &prefix, transaction_id, &abort_error);
			printf("forward compared with %s => cmp(%d), prefix(%d)\n\n", compare_with[i], cmp, prefix);
			delete_binary_read_iterator(bri1, transaction_id, &abort_error);
			delete_binary_read_iterator(bri2, transaction_id, &abort_error);
		}

		{
			datum uval;
			const data_type_info* dti = get_type_info_for_element_from_tuple_def(tpl_d, ACCS);
			get_value_from_element_from_tuple(&uval, tpl_d, ACCS, inline_tuple);
			binary_read_iterator* bri1 = get_new_binary_read_iterator(&uval, dti, &bstd, pam_p, NULL);
			binary_read_iterator* bri2 = get_new_binary_read_iterator((compare_with[i] == NULL) ? (NULL_DATUM) : &((datum){.string_or_binary_value = compare_with[i], .string_or_binary_size = strlen(compare_with[i])}), NULL, &bstd, pam_p, NULL);
			int cmp = 100;
			int prefix = 100;
			cmp = compare_tb(bri2, bri1, &prefix, transaction_id, &abort_error);
			printf("reverse compared with %s => cmp(%d), prefix(%d)\n\n", compare_with[i], cmp, prefix);
			delete_binary_read_iterator(bri1, transaction_id, &abort_error);
			delete_binary_read_iterator(bri2, transaction_id, &abort_error);
		}
	}
	printf("\n\n");

	insert_all_test_data(tpl_d, inline_tuple, &bstd, pam_p, pmm_p);
	read_and_compare_all_test_data(tpl_d, inline_tuple, &bstd, pam_p);

	for(int i = 0; i < sizeof(compare_with)/sizeof(compare_with[0]); i++)
	{
		{
			datum uval;
			const data_type_info* dti = get_type_info_for_element_from_tuple_def(tpl_d, ACCS);
			get_value_from_element_from_tuple(&uval, tpl_d, ACCS, inline_tuple);
			binary_read_iterator* bri1 = get_new_binary_read_iterator(&uval, dti, &bstd, pam_p, NULL);
			binary_read_iterator* bri2 = get_new_binary_read_iterator((compare_with[i] == NULL) ? (NULL_DATUM) : &((datum){.string_or_binary_value = compare_with[i], .string_or_binary_size = strlen(compare_with[i])}), NULL, &bstd, pam_p, NULL);
			int cmp = 100;
			int prefix = 100;
			cmp = compare_tb(bri1, bri2, &prefix, transaction_id, &abort_error);
			printf("forward compared with %s => cmp(%d), prefix(%d)\n\n", compare_with[i], cmp, prefix);
			delete_binary_read_iterator(bri1, transaction_id, &abort_error);
			delete_binary_read_iterator(bri2, transaction_id, &abort_error);
		}

		{
			datum uval;
			const data_type_info* dti = get_type_info_for_element_from_tuple_def(tpl_d, ACCS);
			get_value_from_element_from_tuple(&uval, tpl_d, ACCS, inline_tuple);
			binary_read_iterator* bri1 = get_new_binary_read_iterator(&uval, dti, &bstd, pam_p, NULL);
			binary_read_iterator* bri2 = get_new_binary_read_iterator((compare_with[i] == NULL) ? (NULL_DATUM) : &((datum){.string_or_binary_value = compare_with[i], .string_or_binary_size = strlen(compare_with[i])}), NULL, &bstd, pam_p, NULL);
			int cmp = 100;
			int prefix = 100;
			cmp = compare_tb(bri2, bri1, &prefix, transaction_id, &abort_error);
			printf("reverse compared with %s => cmp(%d), prefix(%d)\n\n", compare_with[i], cmp, prefix);
			delete_binary_read_iterator(bri1, transaction_id, &abort_error);
			delete_binary_read_iterator(bri2, transaction_id, &abort_error);
		}
	}
	printf("\n\n");

	// run comparison based tests
	compare_tests(&bstd, pam_p, pmm_p);

	/* TESTS ENDED */

	/* CLEANUP */

	// destroy blob_store
	destroy_blob_store(blob_store_root_page_id, &bstd, pam_p, transaction_id, &abort_error);
	if(abort_error)
	{
		printf("ABORTED\n");
		exit(-1);
	}

	// destroy the notifier
	deinitialize_heap_table_accumulative_notifier(&htan);

	// close the in-memory data store
	close_and_destroy_unWALed_in_memory_data_store(pam_p);

	// destroy blob_store_tuple_definitions
	deinit_blob_store_tuple_definitions(&bstd);

	// destory page_modification_methods
	delete_unWALed_page_modification_methods(pmm_p);

	// delete the record definition
	free(short_dti);
	free(large_dti);

	return 0;
}

void set_and_compare(const char* s1, const char* s2, char* tuple, const tuple_def* tpl_d, blob_store_tuple_defs* bstd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	init_tuple(tpl_d, tuple);

	// set s1 in tuple
	if(s1 != NULL)
	{
		set_element_in_tuple(tpl_d, STATIC_POSITION(0), tuple, EMPTY_DATUM, UINT32_MAX);
		binary_write_iterator* tbwi_p = get_new_binary_write_iterator(tuple, tpl_d, STATIC_POSITION(0), blob_store_root_page_id, get_NULL_tuple_pointer(&(pam_p->pas)), PREFIX_SIZE, bstd_p, pam_p, pmm_p);

		const char* bytes = s1;
		uint32_t bytes_to_write = strlen(s1);
		while(bytes_to_write > 0)
		{
			uint32_t bytes_to_write_this_iteration = bytes_to_write;
			bytes_to_write_this_iteration = append_to_binary_write_iterator(tbwi_p, bytes, bytes_to_write_this_iteration, &HEAP_TABLE_ACCUMULATIVE_NOTIFIER(&htan), transaction_id, &abort_error);

			fix_all_entries(&(bstd_p->httd), pam_p, pmm_p);

			if(bytes_to_write_this_iteration == 0)
				break;

			bytes += bytes_to_write_this_iteration;
			bytes_to_write -= bytes_to_write_this_iteration;
		}

		delete_binary_write_iterator(tbwi_p, transaction_id, &abort_error);
	}

	// set s2 in tuple
	if(s2 != NULL)
	{
		set_element_in_tuple(tpl_d, STATIC_POSITION(1), tuple, EMPTY_DATUM, UINT32_MAX);
		binary_write_iterator* tbwi_p = get_new_binary_write_iterator(tuple, tpl_d, STATIC_POSITION(1), blob_store_root_page_id, get_NULL_tuple_pointer(&(pam_p->pas)), PREFIX_SIZE, bstd_p, pam_p, pmm_p);

		const char* bytes = s2;
		uint32_t bytes_to_write = strlen(s2);
		while(bytes_to_write > 0)
		{
			uint32_t bytes_to_write_this_iteration = bytes_to_write;
			bytes_to_write_this_iteration = append_to_binary_write_iterator(tbwi_p, bytes, bytes_to_write_this_iteration, &HEAP_TABLE_ACCUMULATIVE_NOTIFIER(&htan), transaction_id, &abort_error);

			fix_all_entries(&(bstd_p->httd), pam_p, pmm_p);

			if(bytes_to_write_this_iteration == 0)
				break;

			bytes += bytes_to_write_this_iteration;
			bytes_to_write -= bytes_to_write_this_iteration;
		}

		delete_binary_write_iterator(tbwi_p, transaction_id, &abort_error);
	}

	printf("\nprinting the built tuple : \n");
	print_tuple(tuple, tpl_d);
	printf("\n");

	{
		int cmp = 100;
		int prefix = 100;
		datum uval;
		const data_type_info* dti;
		dti = get_type_info_for_element_from_tuple_def(tpl_d, STATIC_POSITION(0));
		get_value_from_element_from_tuple(&uval, tpl_d, STATIC_POSITION(0), tuple);
		binary_read_iterator* bri1 = get_new_binary_read_iterator(&uval, dti, bstd_p, pam_p, NULL);
		dti = get_type_info_for_element_from_tuple_def(tpl_d, STATIC_POSITION(1));
		get_value_from_element_from_tuple(&uval, tpl_d, STATIC_POSITION(1), tuple);
		binary_read_iterator* bri2 = get_new_binary_read_iterator(&uval, dti, bstd_p, pam_p, NULL);
		cmp = compare_tb(bri1, bri2, &prefix, transaction_id, &abort_error);
		printf("%s, %s => cmp(%d), prefix(%d)\n", s1, s2, cmp, prefix);
		delete_binary_read_iterator(bri1, transaction_id, &abort_error);
		delete_binary_read_iterator(bri2, transaction_id, &abort_error);
	}

	{
		int cmp = 100;
		int prefix = 100;
		datum uval;
		const data_type_info* dti;
		dti = get_type_info_for_element_from_tuple_def(tpl_d, STATIC_POSITION(0));
		get_value_from_element_from_tuple(&uval, tpl_d, STATIC_POSITION(0), tuple);
		binary_read_iterator* bri1 = get_new_binary_read_iterator(&uval, dti, bstd_p, pam_p, NULL);
		dti = get_type_info_for_element_from_tuple_def(tpl_d, STATIC_POSITION(1));
		get_value_from_element_from_tuple(&uval, tpl_d, STATIC_POSITION(1), tuple);
		binary_read_iterator* bri2 = get_new_binary_read_iterator(&uval, dti, bstd_p, pam_p, NULL);
		cmp = compare_tb(bri2, bri1, &prefix, transaction_id, &abort_error);
		printf("%s, %s => cmp(%d), prefix(%d)\n", s2, s1, cmp, prefix);
		delete_binary_read_iterator(bri1, transaction_id, &abort_error);
		delete_binary_read_iterator(bri2, transaction_id, &abort_error);
	}
	printf("\n");
}

void compare_tests(blob_store_tuple_defs* bstd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	char tuple[1024];

	{
		initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 2);
		strcpy(tuple_dti->containees[0].field_name, "containee1");
		strcpy(tuple_dti->containees[1].field_name, "containee2");
		tuple_dti->containees[0].al.type_info = short_dti;
		tuple_dti->containees[1].al.type_info = short_dti;
		initialize_tuple_def(&tpl_d, tuple_dti);

		set_and_compare(NULL, NULL, tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare(NULL, "abc", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("def", NULL, tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare(NULL, "", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("", NULL, tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("", "", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("abc", "def", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("abcd", "abce", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("abc", "abcd", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
	}

	{
		initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 2);
		strcpy(tuple_dti->containees[0].field_name, "containee1");
		strcpy(tuple_dti->containees[1].field_name, "containee2");
		tuple_dti->containees[0].al.type_info = large_dti;
		tuple_dti->containees[1].al.type_info = short_dti;
		initialize_tuple_def(&tpl_d, tuple_dti);

		set_and_compare(NULL, NULL, tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare(NULL, "abc", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("def", NULL, tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare(NULL, "", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("", NULL, tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("", "", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("abc", "def", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("abcd", "abce", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("abc", "abcd", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
	}

	{
		initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 2);
		strcpy(tuple_dti->containees[0].field_name, "containee1");
		strcpy(tuple_dti->containees[1].field_name, "containee2");
		tuple_dti->containees[0].al.type_info = large_dti;
		tuple_dti->containees[1].al.type_info = large_dti;
		initialize_tuple_def(&tpl_d, tuple_dti);

		set_and_compare(NULL, NULL, tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare(NULL, "abc", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("def", NULL, tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare(NULL, "", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("", NULL, tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("", "", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("abc", "def", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("abcd", "abce", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
		set_and_compare("abc", "abcd", tuple, &tpl_d, bstd_p, pam_p, pmm_p);
	}
}