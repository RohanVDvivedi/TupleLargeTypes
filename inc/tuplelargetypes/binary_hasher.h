#ifndef BINARY_HASHER_H
#define BINARY_HASHER_H

#include<tuplestore/tuple_def.h>
#include<tuplestore/tuple.h>

#include<tupleindexer/blob_store/blob_store.h>
#include<tupleindexer/interface/page_access_methods.h>

#include<tuplelargetypes/extension_reader_iterator_callback.h>

uint64_t hash_tbn(const datum* uval, const data_type_info* dti, tuple_hasher* th, const blob_store_tuple_defs* bstd_p, const page_access_methods* pam_p, const void* transaction_id, int* abort_error, extension_reader_iterator_callback* callback);

#define hash_text hash_tbn
#define hash_blob hash_tbn
#define hash_numeric hash_tbn

#endif