#include<stdio.h>
#include<stdlib.h>

#include<tuple.h>
#include<tuple_def.h>

#include<unWALed_in_memory_data_store.h>
#include<unWALed_page_modification_methods.h>

#include<blob_large.h>
#include<text_large.h>

//#define USE_SHORT
#define USE_LARGE

#define USE_BASE
//#define USE_NESTED

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

char* test_data = "Rohan is a good boy,"
					"Rohan is probably a bad boy,"
					"Common sense is not all that common,"
					"Does Rohan have any?"
					"Not if this code functions as required,"
					"What else can I write here?"
					"No one cares what would be written here."
					"I will probably change it to lorem ipsum once this project goes big enough";

tuple_def tpl_d;
data_type_info* short_dti = NULL;
data_type_info* large_dti = NULL;
char tuple_type_info_memory[sizeof_tuple_data_type_info(2)];
data_type_info* tuple_dti = (data_type_info*)tuple_type_info_memory;
tuple_def* get_tuple_definition(const page_access_specs* pas_p)
{
	short_dti = get_text_short_type_info(PREFIX_SIZE + 10);
	large_dti = get_text_large_type_info(short_dti, pas_p);

	initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 1);
	strcpy(tuple_dti->containees[0].field_name, "containee");
	#ifdef USE_SHORT
		tuple_dti->containees[0].al.type_info = short_dti;
	#else
		tuple_dti->containees[0].al.type_info = large_dti;
	#endif

	data_type_info* dti = NULL;
	#ifdef USE_NESTED
		dti = tuple_dti;
	#elif defined USE_BASE
		#ifdef USE_SHORT
			dti = short_dti;
		#elif defined USE_LARGE
			dti = large_dti;
		#endif
	#endif

	initialize_tuple_def(&tpl_d, dti);
	return &tpl_d;
}

void insert_all_test_data(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	text_blob_write_iterator* tbwi_p = get_new_text_blob_write_iterator(inline_tuple, tpl_d, ACCS, PREFIX_SIZE, wtd_p, pam_p, pmm_p);

	char* bytes = test_data;
	uint32_t bytes_to_write = strlen(test_data);
	while(bytes_to_write > 0)
	{
		uint32_t bytes_to_write_this_iteration = min(bytes_to_write, WRITE_CHUNK_SIZE);

		bytes_to_write_this_iteration = append_to_text_blob(tbwi_p, bytes, bytes_to_write_this_iteration, transaction_id, &abort_error);

		if(bytes_to_write_this_iteration == 0)
			break;

		bytes += bytes_to_write_this_iteration;
		bytes_to_write -= bytes_to_write_this_iteration;
	}

	printf("bytes_written = %"PRIu32"/%"PRIu32"\n", bytes_to_write, (uint32_t)strlen(test_data));

	delete_text_blob_write_iterator(tbwi_p, transaction_id, &abort_error);
}

void read_and_compare_all_test_data(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p)
{
	text_blob_read_iterator* tbri_p = get_new_text_blob_read_iterator(inline_tuple, tpl_d, ACCS, wtd_p, pam_p);

	char read_buffer[READ_CHUNK_SIZE];
	uint32_t bytes_read = 0;
	while(1)
	{
		uint32_t bytes_read_this_iteration = read_from_text_blob(tbri_p, read_buffer, READ_CHUNK_SIZE, transaction_id, &abort_error);
		if(bytes_read_this_iteration == 0)
			break;

		printf(" ->\"%.*s\"\n", bytes_read_this_iteration, read_buffer);

		bytes_read += bytes_read_this_iteration;
	}

	printf("bytes_read = %"PRIu32"/%"PRIu32"\n", bytes_read, (uint32_t)strlen(test_data));

	delete_text_blob_read_iterator(tbri_p, transaction_id, &abort_error);
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

	/* TESTS STARTED */

	char inline_tuple[PAGE_SIZE];
	init_tuple(tpl_d, inline_tuple);

	insert_all_test_data(tpl_d, inline_tuple, &wtd, pam_p, pmm_p);
	read_and_compare_all_test_data(tpl_d, inline_tuple, &wtd, pam_p);

	insert_all_test_data(tpl_d, inline_tuple, &wtd, pam_p, pmm_p);
	read_and_compare_all_test_data(tpl_d, inline_tuple, &wtd, pam_p);

	/* TESTS ENDED */

	/* CLEANUP */

	// destroy worm
	// TODO

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