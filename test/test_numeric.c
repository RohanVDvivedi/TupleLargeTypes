#include<stdio.h>
#include<stdlib.h>

#include<tuplestore/tuple.h>
#include<tuplestore/tuple_def.h>

#include<tupleindexer/interface/unWALed_in_memory_data_store.h>
#include<tupleindexer/interface/unWALed_page_modification_methods.h>

#include<tuplelargetypes/numeric_extended.h>

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

#define PREFIX_SIZE 9

#define READ_CHUNK_SIZE 7
//#define READ_CHUNK_SIZE 30

#define WRITE_CHUNK_SIZE 7
//#define WRITE_CHUNK_SIZE 30

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
	uint32_t max_size = 1 + 1 + 2 + 1 + PREFIX_SIZE * 5 + 20;
	short_dti = get_numeric_inline_type_info(max_size); // giving it 10 extra bytes, 2 digits extra
	large_dti = get_numeric_extended_type_info(max_size, short_dti, pas_p);

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

#define TEST_DIGITS_COUNT 200
#define TEST_DIGIT(index) (hash_randomizer((index)) % 1000000000000ULL)
void populate_digits_buffer(uint64_t* digits, uint32_t index, uint32_t count)
{
	for(uint32_t i = 0; i < count; i++)
		digits[i] = TEST_DIGIT((index + i) % TEST_DIGITS_COUNT);
}

void insert_all_test_digits(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	printf("INLINE TUPLE (before init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));

	digit_write_iterator* dwi_p = get_new_digit_write_iterator(inline_tuple, tpl_d, ACCS, PREFIX_SIZE, wtd_p, pam_p, pmm_p);

	printf("INLINE TUPLE (after init-ing write_iterator) : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));

	uint64_t digits[WRITE_CHUNK_SIZE];
	uint32_t digits_to_write = TEST_DIGITS_COUNT;
	uint32_t digits_written = 0;
	while(digits_to_write > 0)
	{
		uint32_t digits_to_write_this_iteration = min(digits_to_write, WRITE_CHUNK_SIZE);

		populate_digits_buffer(digits, digits_written, digits_to_write_this_iteration);
		digits_to_write_this_iteration = append_to_digit_write_iterator(dwi_p, digits, digits_to_write_this_iteration, transaction_id, &abort_error);

		if(digits_to_write_this_iteration == 0)
			break;

		printf("digits_written_this_iteration = %"PRIu32"\n", digits_to_write_this_iteration);

		digits_to_write -= digits_to_write_this_iteration;
		digits_written += digits_to_write_this_iteration;
	}

	printf("digits_written = %"PRIu32"/%"PRIu32"\n\n", digits_written, TEST_DIGITS_COUNT);

	delete_digit_write_iterator(dwi_p, transaction_id, &abort_error);
}

void read_and_compare_all_test_digits(tuple_def* tpl_d, char* inline_tuple, worm_tuple_defs* wtd_p, page_access_methods* pam_p)
{
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));
	printf("hash => %"PRIu64"\n\n", hash_numeric(tpl_d, inline_tuple, ACCS, FNV_64_TUPLE_HASHER, wtd_p, pam_p, transaction_id, &abort_error));

	digit_read_iterator* dri_p = get_new_digit_read_iterator(inline_tuple, tpl_d, ACCS, wtd_p, pam_p);

	uint64_t digits[READ_CHUNK_SIZE];

	uint64_t read_buffer[READ_CHUNK_SIZE];
	uint32_t digits_read = 0;
	while(1)
	{
		uint32_t digits_read_this_iteration = read_from_digit_read_iterator(dri_p, read_buffer, READ_CHUNK_SIZE, transaction_id, &abort_error);
		if(digits_read_this_iteration == 0)
			break;

		populate_digits_buffer(digits, digits_read, digits_read_this_iteration);

		printf(" ->");
		int matches = 1;
		for(uint32_t i = 0; i < digits_read_this_iteration; i++)
		{
			matches = matches && (read_buffer[i] == digits[i]);
			printf("%"PRIu64", ", read_buffer[i]);
		}

		printf("matches => %d\n", matches);

		digits_read += digits_read_this_iteration;
	}

	printf("digits_read = %"PRIu32"/%"PRIu32"\n\n", digits_read, TEST_DIGITS_COUNT);

	delete_digit_read_iterator(dri_p, transaction_id, &abort_error);
}

void compare_tests(worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p);

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
	printf(" worm -> %"PRIu64"\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));
	printf("\n");
	read_and_compare_all_test_digits(tpl_d, inline_tuple, &wtd, pam_p);

	s = NEGATIVE_NUMERIC;
	e = -2;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = POSITIVE_INFINITY_NUMERIC;
	e = -2;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = NEGATIVE_NUMERIC;
	e = 2;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = ZERO_NUMERIC;
	e = 1;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = ZERO_NUMERIC;
	e = 0;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = POSITIVE_NUMERIC;
	e = 10;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = POSITIVE_NUMERIC;
	e = 10;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = POSITIVE_INFINITY_NUMERIC;
	e = 10;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));
	printf("\n");

	s = NEGATIVE_NUMERIC;
	e = 2;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf("\n");

	s = NEGATIVE_INFINITY_NUMERIC;
	e = -5;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));
	printf("\n");

	s = POSITIVE_NUMERIC;
	e = 5;
	set_sign_bits_and_exponent_for_numeric(s, e, inline_tuple, tpl_d, ACCS);
	printf("INLINE TUPLE : ");
	print_tuple(inline_tuple, tpl_d);
	printf(" worm -> %"PRIu64"\n", get_extension_head_page_id_for_extended_type(inline_tuple, tpl_d, ACCS, &(pam_p->pas)));
	printf("\n");

	read_and_compare_all_test_digits(tpl_d, inline_tuple, &wtd, pam_p);

	insert_all_test_digits(tpl_d, inline_tuple, &wtd, pam_p, pmm_p);
	read_and_compare_all_test_digits(tpl_d, inline_tuple, &wtd, pam_p);

	#define init_static_mat_num(n, s, e, ds) initialize_static_materialized_numeric(n, s, e, ds, sizeof(ds)/sizeof(uint64_t))
	materialized_numeric m1;init_static_mat_num(&m1, NEGATIVE_INFINITY_NUMERIC, 5, ((uint64_t[]){1,2,3}));
	materialized_numeric m2;init_static_mat_num(&m2, ZERO_NUMERIC, 5, ((uint64_t[]){1,2,3}));
	materialized_numeric m3;init_static_mat_num(&m3, POSITIVE_INFINITY_NUMERIC, 5, ((uint64_t[]){1,2,3}));
	materialized_numeric m4;init_static_mat_num(&m4, POSITIVE_NUMERIC, 5, ((uint64_t[]){TEST_DIGIT(0),TEST_DIGIT(1),TEST_DIGIT(2)}));
	materialized_numeric m5;init_static_mat_num(&m5, NEGATIVE_NUMERIC, 5, ((uint64_t[]){TEST_DIGIT(0),TEST_DIGIT(1),TEST_DIGIT(2)}));
	materialized_numeric m6;init_static_mat_num(&m6, POSITIVE_NUMERIC, 5, ((uint64_t[]){TEST_DIGIT(0),TEST_DIGIT(1),TEST_DIGIT(2),5}));
	materialized_numeric m7;init_static_mat_num(&m7, NEGATIVE_NUMERIC, 5, ((uint64_t[]){TEST_DIGIT(0),TEST_DIGIT(1),TEST_DIGIT(2),1}));

	uint64_t m8_digits[TEST_DIGITS_COUNT];
	materialized_numeric m8;
	populate_digits_buffer(m8_digits, 0, TEST_DIGITS_COUNT);
	initialize_static_materialized_numeric(&m8, POSITIVE_NUMERIC, 5, m8_digits, TEST_DIGITS_COUNT);

	uint64_t m9_digits[TEST_DIGITS_COUNT * 2];
	materialized_numeric m9;
	populate_digits_buffer(m9_digits, 0, TEST_DIGITS_COUNT * 2);
	initialize_static_materialized_numeric(&m9, POSITIVE_NUMERIC, 5, m9_digits, TEST_DIGITS_COUNT * 2);

	uint64_t m10_digits[TEST_DIGITS_COUNT * 3];
	materialized_numeric m10;
	populate_digits_buffer(m10_digits, 0, TEST_DIGITS_COUNT * 3);
	initialize_static_materialized_numeric(&m10, POSITIVE_NUMERIC, 5, m10_digits, TEST_DIGITS_COUNT * 3);

	materialized_numeric* compare_with[] = {
		NULL,
		&m1,
		&m2,
		&m3,
		&m4,
		&m5,
		&m6,
		&m7,
		&m8,
		&m9,
		&m10,
	};

	for(int i = 0; i < sizeof(compare_with)/sizeof(compare_with[0]); i++)
	{
		{
			numeric_reader_interface nri1 = init_intuple_numeric_reader_interface(tpl_d, inline_tuple, ACCS, &wtd, pam_p, transaction_id, &abort_error);
			numeric_reader_interface nri2 = init_materialized_numeric_reader_interface(compare_with[i]);
			int cmp = 100;
			int prefix = 100;
			cmp = compare_numeric(&nri1, &nri2, &prefix);
			printf("forward compared with ");
			if(compare_with[i] != NULL)
				print_materialized_numeric(compare_with[i]);
			else
				printf("NULL");
			printf(" => cmp(%d), prefix(%d)\n\n", cmp, prefix);
		}

		{
			numeric_reader_interface nri1 = init_intuple_numeric_reader_interface(tpl_d, inline_tuple, ACCS, &wtd, pam_p, transaction_id, &abort_error);
			numeric_reader_interface nri2 = init_materialized_numeric_reader_interface(compare_with[i]);
			int cmp = 100;
			int prefix = 100;
			cmp = compare_numeric(&nri2, &nri1, &prefix);
			printf("reverse compared with ");
			if(compare_with[i] != NULL)
				print_materialized_numeric(compare_with[i]);
			else
				printf("NULL");
			printf(" => cmp(%d), prefix(%d)\n\n", cmp, prefix);
		}
	}

	insert_all_test_digits(tpl_d, inline_tuple, &wtd, pam_p, pmm_p);
	read_and_compare_all_test_digits(tpl_d, inline_tuple, &wtd, pam_p);

	for(int i = 0; i < sizeof(compare_with)/sizeof(compare_with[0]); i++)
	{
		{
			numeric_reader_interface nri1 = init_intuple_numeric_reader_interface(tpl_d, inline_tuple, ACCS, &wtd, pam_p, transaction_id, &abort_error);
			numeric_reader_interface nri2 = init_materialized_numeric_reader_interface(compare_with[i]);
			int cmp = 100;
			int prefix = 100;
			cmp = compare_numeric(&nri1, &nri2, &prefix);
			printf("forward compared with ");
			if(compare_with[i] != NULL)
				print_materialized_numeric(compare_with[i]);
			else
				printf("NULL");
			printf(" => cmp(%d), prefix(%d)\n\n", cmp, prefix);
		}

		{
			numeric_reader_interface nri1 = init_intuple_numeric_reader_interface(tpl_d, inline_tuple, ACCS, &wtd, pam_p, transaction_id, &abort_error);
			numeric_reader_interface nri2 = init_materialized_numeric_reader_interface(compare_with[i]);
			int cmp = 100;
			int prefix = 100;
			cmp = compare_numeric(&nri2, &nri1, &prefix);
			printf("reverse compared with ");
			if(compare_with[i] != NULL)
				print_materialized_numeric(compare_with[i]);
			else
				printf("NULL");
			printf(" => cmp(%d), prefix(%d)\n\n", cmp, prefix);
		}
	}

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
	free(short_dti->containees[2].al.type_info);
	free(short_dti);
	free(large_dti);

	return 0;
}

typedef struct num num;
struct num
{
	numeric_sign_bits sb;
	int16_t exp;
	uint32_t digits_count;
	uint64_t* digits;
};

#define STATIC_NUM(sb_, exp_, digits_) &((num){.sb = sb_, .exp = exp_, .digits_count = sizeof(digits_)/sizeof(uint64_t), .digits = digits_})
#define STATIC_NUM2(sb_, exp_) &((num){.sb = sb_, .exp = exp_, .digits_count = 0, .digits = NULL})

void print_num(const num* n)
{
	if(n == NULL)
	{
		printf("NULL");
		return;
	}
	printf("%s %"PRId16" [", numeric_sign_bits_str[n->sb], n->exp);
	if(n->digits == NULL)
		printf("NULL]");
	else
	{
		for(uint32_t i = 0; i < n->digits_count; i++)
			printf("%"PRIu64", ", n->digits[i]);
		printf("]");
	}
}

void set_and_compare(const num* n1, const num* n2, char* tuple, const tuple_def* tpl_d, worm_tuple_defs* wtd_p, page_access_methods* pam_p, page_modification_methods* pmm_p)
{
	init_tuple(tpl_d, tuple);

	// set s1 in tuple
	if(n1 != NULL)
	{
		set_element_in_tuple(tpl_d, STATIC_POSITION(0), tuple, EMPTY_USER_VALUE, UINT32_MAX);
		set_sign_bits_and_exponent_for_numeric(n1->sb, n1->exp, tuple, tpl_d, STATIC_POSITION(0));

		if(n1->digits != NULL)
		{
			digit_write_iterator* dwi_p = get_new_digit_write_iterator(tuple, tpl_d, STATIC_POSITION(0), PREFIX_SIZE, wtd_p, pam_p, pmm_p);

			const uint64_t* digits = n1->digits;
			uint32_t digits_to_write = n1->digits_count;
			while(digits_to_write > 0)
			{
				uint32_t digits_to_write_this_iteration = digits_to_write;
				digits_to_write_this_iteration = append_to_digit_write_iterator(dwi_p, digits, digits_to_write_this_iteration, transaction_id, &abort_error);

				if(digits_to_write_this_iteration == 0)
					break;

				digits += digits_to_write_this_iteration;
				digits_to_write -= digits_to_write_this_iteration;
			}

			delete_digit_write_iterator(dwi_p, transaction_id, &abort_error);
		}
	}

	// set s2 in tuple
	if(n2 != NULL)
	{
		set_element_in_tuple(tpl_d, STATIC_POSITION(1), tuple, EMPTY_USER_VALUE, UINT32_MAX);
		set_sign_bits_and_exponent_for_numeric(n2->sb, n2->exp, tuple, tpl_d, STATIC_POSITION(1));

		if(n2->digits != NULL)
		{
			digit_write_iterator* dwi_p = get_new_digit_write_iterator(tuple, tpl_d, STATIC_POSITION(1), PREFIX_SIZE, wtd_p, pam_p, pmm_p);

			const uint64_t* digits = n2->digits;
			uint32_t digits_to_write = n2->digits_count;
			while(digits_to_write > 0)
			{
				uint32_t digits_to_write_this_iteration = digits_to_write;
				digits_to_write_this_iteration = append_to_digit_write_iterator(dwi_p, digits, digits_to_write_this_iteration, transaction_id, &abort_error);

				if(digits_to_write_this_iteration == 0)
					break;

				digits += digits_to_write_this_iteration;
				digits_to_write -= digits_to_write_this_iteration;
			}

			delete_digit_write_iterator(dwi_p, transaction_id, &abort_error);
		}
	}

	printf("\nprinting the built tuple : \n");
	print_tuple(tuple, tpl_d);
	printf("\n");

	{
		int cmp = 100;
		int prefix = 100;
		numeric_reader_interface nri1 = init_intuple_numeric_reader_interface(tpl_d, tuple, STATIC_POSITION(0), wtd_p, pam_p, transaction_id, &abort_error);
		numeric_reader_interface nri2 = init_intuple_numeric_reader_interface(tpl_d, tuple, STATIC_POSITION(1), wtd_p, pam_p, transaction_id, &abort_error);
		cmp = compare_numeric(&nri1, &nri2, &prefix);
		print_num(n1);
		printf(", ");
		print_num(n2);
		printf(" => cmp(%d), prefix(%d)\n",cmp, prefix);
	}

	{
		int cmp = 100;
		int prefix = 100;
		numeric_reader_interface nri1 = init_intuple_numeric_reader_interface(tpl_d, tuple, STATIC_POSITION(0), wtd_p, pam_p, transaction_id, &abort_error);
		numeric_reader_interface nri2 = init_intuple_numeric_reader_interface(tpl_d, tuple, STATIC_POSITION(1), wtd_p, pam_p, transaction_id, &abort_error);
		cmp = compare_numeric(&nri2, &nri1, &prefix);
		print_num(n2);
		printf(", ");
		print_num(n1);
		printf(" => cmp(%d), prefix(%d)\n",cmp, prefix);
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

	{
		initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 2);
		strcpy(tuple_dti->containees[0].field_name, "containee1");
		strcpy(tuple_dti->containees[1].field_name, "containee2");
		tuple_dti->containees[0].al.type_info = short_dti;
		tuple_dti->containees[1].al.type_info = short_dti;
		initialize_tuple_def(&tpl_d, tuple_dti);

		set_and_compare(NULL, NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(NULL, STATIC_NUM2(NEGATIVE_INFINITY_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(NEGATIVE_INFINITY_NUMERIC, 0), STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3})), STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3,5})), STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, 0, ((uint64_t[]){1,2})), STATIC_NUM2(ZERO_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(ZERO_NUMERIC, 0), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,5})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,3})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,5})), STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,3})), STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(ZERO_NUMERIC, 0), STATIC_NUM2(ZERO_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,5})), STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		printf("\n\n========================================\n\n");
	}

	{
		initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 2);
		strcpy(tuple_dti->containees[0].field_name, "containee1");
		strcpy(tuple_dti->containees[1].field_name, "containee2");
		tuple_dti->containees[0].al.type_info = large_dti;
		tuple_dti->containees[1].al.type_info = short_dti;
		initialize_tuple_def(&tpl_d, tuple_dti);

		set_and_compare(NULL, NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(NULL, STATIC_NUM2(NEGATIVE_INFINITY_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(NEGATIVE_INFINITY_NUMERIC, 0), STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3})), STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3,5})), STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, 0, ((uint64_t[]){1,2})), STATIC_NUM2(ZERO_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(ZERO_NUMERIC, 0), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,5})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,3})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,5})), STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,3})), STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(ZERO_NUMERIC, 0), STATIC_NUM2(ZERO_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,5})), STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		printf("\n\n========================================\n\n");
	}

	{
		initialize_tuple_data_type_info(tuple_dti, "container", 1, PAGE_SIZE, 2);
		strcpy(tuple_dti->containees[0].field_name, "containee1");
		strcpy(tuple_dti->containees[1].field_name, "containee2");
		tuple_dti->containees[0].al.type_info = large_dti;
		tuple_dti->containees[1].al.type_info = large_dti;
		initialize_tuple_def(&tpl_d, tuple_dti);

		set_and_compare(NULL, NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(NULL, STATIC_NUM2(NEGATIVE_INFINITY_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), NULL, tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(NEGATIVE_INFINITY_NUMERIC, 0), STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3})), STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3,5})), STATIC_NUM(NEGATIVE_NUMERIC, 3, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, 0, ((uint64_t[]){1,2})), STATIC_NUM2(ZERO_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(ZERO_NUMERIC, 0), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,5})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,3})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,5})), STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,3})), STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), STATIC_NUM(NEGATIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,4})), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM2(ZERO_NUMERIC, 0), STATIC_NUM2(ZERO_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		set_and_compare(STATIC_NUM(POSITIVE_NUMERIC, -5, ((uint64_t[]){1,2,3,5})), STATIC_NUM2(POSITIVE_INFINITY_NUMERIC, 0), tuple, &tpl_d, wtd_p, pam_p, pmm_p);
		printf("\n\n========================================\n\n");
	}
}