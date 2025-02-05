#ifndef TEXT_SHORT_H
#define TEXT_SHORT_H

#include<data_type_info.h>

// returns a new type info pointing to STRING type that is atmost max_size bytes big
data_type_info* get_text_short_type_info(uint32_t max_size);

#endif