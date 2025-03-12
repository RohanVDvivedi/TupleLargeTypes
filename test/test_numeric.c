#include<stdio.h>
#include<stdlib.h>

#include<tuple.h>
#include<tuple_def.h>

#include<unWALed_in_memory_data_store.h>
#include<unWALed_page_modification_methods.h>

#include<numeric_extended.h>
#include<worm.h> // to be removed in favor of digit_read_iterator.h and digit_write_iterator.h

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

tuple_def tpl_d;
data_type_info* short_dti = NULL;
data_type_info* large_dti = NULL;
char tuple_type_info_memory[sizeof_tuple_data_type_info(2)];
data_type_info* tuple_dti = (data_type_info*)tuple_type_info_memory;
tuple_def* get_tuple_definition(const page_access_specs* pas_p)
{
	short_dti = get_numeric_inline_type_info(1 + 1 + 2 + 1 + PREFIX_SIZE + 10); // giving it 10 extra bytes, 2 digits extra
	large_dti = get_numeric_extended_type_info(short_dti, pas_p);

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

int main()
{
	// base cases for comparing numeric based on sign bits and exponent

	for(int s1 = 0; s1 < 5; s1++)
	{
		for(int e1 = -3; e1 <= 3; e1 += 3)
		{
			for(int s2 = 0; s2 < 5; s2++)
			{
				for(int e2 = -3; e2 <= 3; e2 += 3)
				{
					int d = 0;
					int cmp = compare_numeric_prefix_no_digits(s1, e1, s2, e2, &d);
					printf("(%s, %d) - (%s, %d) = %d (%d)\n", numeric_sign_bits_str[s1], e1, numeric_sign_bits_str[s2], e2, cmp, d);
				}
			}
		}
	}
	printf("\n");

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

	numeric_sign_bits s;
	int16_t e;

	char inline_tuple[PAGE_SIZE];
	init_tuple(tpl_d, inline_tuple);
	#ifdef USE_NESTED
		set_element_in_tuple(tpl_d, ACCS, inline_tuple, EMPTY_USER_VALUE, UINT32_MAX);
	#endif
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = POSITIVE_INFINITY_NUMERIC;
	e = -2;
	set_sign_bits_and_exponent_for_numeric(&s, &e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = NEGATIVE_NUMERIC;
	e = 2;
	set_sign_bits_and_exponent_for_numeric(&s, &e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = ZERO_NUMERIC;
	e = 1;
	set_sign_bits_and_exponent_for_numeric(NULL, &e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = POSITIVE_NUMERIC;
	e = 10;
	set_sign_bits_and_exponent_for_numeric(&s, NULL, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

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
	free(short_dti);
	free(large_dti);

	return 0;
}