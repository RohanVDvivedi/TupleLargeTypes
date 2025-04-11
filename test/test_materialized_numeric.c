#include<stdio.h>
#include<stdlib.h>

#include<tuplelargetypes/materialized_numeric.h>

#define init_static_mat_num(n, s, e, ds) initialize_static_materialized_numeric(n, s, e, ds, sizeof(ds)/sizeof(uint64_t))

int main()
{
	materialized_numeric m;
	init_static_mat_num(&m, NEGATIVE_INFINITY_NUMERIC, 5, ((uint64_t[]){1,2,3}));
	print_materialized_numeric(&m);
	printf("\n\n");

	init_static_mat_num(&m, NEGATIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
	print_materialized_numeric(&m);
	printf("\n\n");

	init_static_mat_num(&m, ZERO_NUMERIC, 5, ((uint64_t[]){1,2,3}));
	print_materialized_numeric(&m);
	printf("\n\n");

	init_static_mat_num(&m, POSITIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
	print_materialized_numeric(&m);
	printf("\n\n");

	init_static_mat_num(&m, POSITIVE_INFINITY_NUMERIC, 5, ((uint64_t[]){1,2,3}));
	print_materialized_numeric(&m);
	printf("\n\n");

	initialize_materialized_numeric(&m, 12);
	set_sign_bits_and_exponent_for_materialized_numeric(&m, POSITIVE_NUMERIC, 6);

	push_msd_in_materialized_numeric(&m, 1);
	print_materialized_numeric(&m);
	printf("\n\n");

	push_lsd_in_materialized_numeric(&m, 2);
	print_materialized_numeric(&m);
	printf("\n\n");

	push_msd_in_materialized_numeric(&m, 3);
	print_materialized_numeric(&m);
	printf("\n\n");

	push_lsd_in_materialized_numeric(&m, 4);
	print_materialized_numeric(&m);
	printf("\n\n");

	{
		materialized_numeric m1;
		initialize_from_materialized_numeric(&m1, NULL, 0, &m);
		print_materialized_numeric(&m1);
		printf(" => clone\n\n");

		deinitialize_materialized_numeric(&m1);
	}

	{
		materialized_numeric m1;
		initialize_from_materialized_numeric(&m1, (uint64_t[5]){}, 5, &m);
		print_materialized_numeric(&m1);
		printf(" => clone\n\n");

		deinitialize_materialized_numeric(&m1);
	}

	pop_lsd_from_materialized_numeric(&m);
	print_materialized_numeric(&m);
	printf("\n\n");

	pop_lsd_from_materialized_numeric(&m);
	print_materialized_numeric(&m);
	printf("\n\n");

	set_sign_bits_and_exponent_for_materialized_numeric(&m, ZERO_NUMERIC, 3);
	print_materialized_numeric(&m);
	printf("\n\n");

	push_lsd_in_materialized_numeric(&m, 2);
	print_materialized_numeric(&m);
	printf("\n\n");

	push_msd_in_materialized_numeric(&m, 3);
	print_materialized_numeric(&m);
	printf("\n\n");

	deinitialize_materialized_numeric(&m);

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, NEGATIVE_INFINITY_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, NEGATIVE_INFINITY_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, POSITIVE_INFINITY_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, POSITIVE_INFINITY_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, ZERO_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, ZERO_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, NEGATIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, NEGATIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, NEGATIVE_NUMERIC, 6, ((uint64_t[]){1,2,3}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, NEGATIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, POSITIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, POSITIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, POSITIVE_NUMERIC, 6, ((uint64_t[]){1,2,3}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, POSITIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, NEGATIVE_NUMERIC, 6, ((uint64_t[]){1,2,3,4}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, POSITIVE_NUMERIC, 6, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, NEGATIVE_NUMERIC, 5, ((uint64_t[]){1,2,3,4}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, POSITIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, NEGATIVE_NUMERIC, 6, ((uint64_t[]){1,2,3,4}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, NEGATIVE_NUMERIC, 6, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	{
		materialized_numeric m1;
		init_static_mat_num(&m1, POSITIVE_NUMERIC, 5, ((uint64_t[]){1,2,3,4}));
		
		materialized_numeric m2;
		init_static_mat_num(&m2, POSITIVE_NUMERIC, 5, ((uint64_t[]){1,2,3}));
		
		int cmp = compare_materialized_numeric(&m1, &m2);

		print_materialized_numeric(&m1);
		printf("\n");
		print_materialized_numeric(&m2);
		printf("\n -> %d\n\n", cmp);
	}

	return 0;
}