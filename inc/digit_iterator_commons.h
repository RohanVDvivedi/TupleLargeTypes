#ifndef DIGIT_ITERATOR_COMMONS_H
#define DIGIT_ITERATOR_COMMONS_H

static inline void point_to_attribute(relative_positional_accessor* rpa, int is_inline)
{
	relative_positonal_accessor_set_from_relative(rpa, SELF);
}

static inline void point_to_prefix_container(relative_positional_accessor* rpa, int is_inline)
{
	if(is_inline)
	{
		relative_positonal_accessor_set_from_relative(rpa, SELF);
		return ;
	}

	relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(0));
}

static inline void point_to_prefix(relative_positional_accessor* rpa, int is_inline)
{
	if(is_inline)
	{
		relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(2));
		return ;
	}

	relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(0, 2));
}

static inline void point_to_prefix_s_digit(relative_positional_accessor* rpa, uint32_t index, int is_inline)
{
	if(is_inline)
	{
		relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(2, index));
		return ;
	}

	relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(0, 2, index));
}

static inline void point_to_extension_head_page_id(relative_positional_accessor* rpa, int is_inline)
{
	if(is_inline)
		return ;

	relative_positonal_accessor_set_from_relative(rpa, STATIC_POSITION(1));
}

#endif