#include<stdio.h>
#include<stdlib.h>

#include<blob_large.h>
#include<text_large.h>

#define USE_TEXT
//#define USE_BLOB

#define USE_BASE
//#define USE_NESTED

#ifdef USE_BASE
	#define ACCS SELF
#else
	#define ACCS STATIC_POSITION(0)
#endif

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
tuple_def* get_tuple_definition()
{
	return NULL;
}

void insert_all_test_data(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{

}

void read_and_compare_all_test_data(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p)
{

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
	tuple_def* tpl_d = get_tuple_definition();

	/* TESTS STARTED */

	char inline_tuple[PAGE_SIZE];

	insert_all_test_data(tpl_d, inline_tuple, wtd_p, pam_p, pmm_p);
	read_and_compare_all_test_data(tpl_d, inline_tuple, wtd_p, pam_p);

	insert_all_test_data(tpl_d, inline_tuple, wtd_p, pam_p, pmm_p);
	read_and_compare_all_test_data(tpl_d, inline_tuple, wtd_p, pam_p);

	/* TESTS ENDED */

	/* CLEANUP */

	// destroy page_table
	destroy_array_table(root_page_id, &attd, pam_p, transaction_id, &abort_error);
	if(abort_error)
	{
		printf("ABORTED\n");
		exit(-1);
	}

	// close the in-memory data store
	close_and_destroy_unWALed_in_memory_data_store(pam_p);

	// destroy worm_tuple_definitions
	deinit_worm_tuple_definitions(&wtd);

	// destory page_modification_methods
	delete_unWALed_page_modification_methods(pmm_p);

	// delete the record definition

	return 0;
}