#ifndef RELATIVE_POSITIONAL_ACCESSOR_H
#define RELATIVE_POSITIONAL_ACCESSOR_H

#include<stdlib.h>

typedef struct relative_positional_accessor relative_positional_accessor;
struct relative_positional_accessor
{
	const positional_accessor* base; // base always remains the same

	positional_accessor exact_position; // exact position this relative_positonal_accessor points to
};

static inline void initialize_relative_positional_accessor(relative_positional_accessor* rpa, const positional_accessor* base, uint32_t max_relative_depth)
{
	(*rpa) = (relative_positional_accessor){};

	rpa->base = base;
	if(max_relative_depth == 0)
		rpa->exact = (*base);
	else
	{
		rpa->exact.positions = malloc((base->positions_length + max_relative_depth) * sizeof(uint32_t));
		if(rpa->exact.positions == NULL)
			exit(-1);
		rpa->exact.positions_length = 0;
	}

	append_positions(&(rpa->exact), *(rpa->base));
}

static inline void relative_positonal_accessor_set_from_relative(relative_positional_accessor* rpa, positional_accessor rel)
{
	rpa->exact.positions_length = rpa->base->positions_length;

	append_positions(&(rpa->exact), rel);
}

static inline void deinitialize_relative_positional_accessor(relative_positional_accessor* rpa)
{
	if(rpa->exact.positions != rpa->base->positions)
		free(rpa->exact.positions);
	(*rpa) = (relative_positional_accessor){};
}

#endif