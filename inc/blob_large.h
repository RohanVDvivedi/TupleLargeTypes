#ifndef BLOB_LARGE_H
#define BLOB_LARGE_H

#include<data_type_info.h>
#include<page_access_specification.h>

// returns a new type info pointing to BLOB type that is atmost max_size bytes big
data_type_info* get_blob_short_type_info(uint32_t max_size);

// returns a new type info pointing to to a tuple, of 2 elements the blob_short_p, and a page_id type from pas_p
data_type_info* get_blob_large_type_info(const data_type_info* blob_short_p, const page_access_specs* pas_p);

#endif