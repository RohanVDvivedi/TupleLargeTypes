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

	return 0;
}