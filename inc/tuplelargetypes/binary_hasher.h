#ifndef BINARY_HASHER_H
#define BINARY_HASHER_H

#include<tuplestore/tuple_def.h>
#include<tuplestore/tuple.h>

#include<tupleindexer/worm/worm.h>
#include<tupleindexer/interface/page_access_methods.h>

uint64_t hash_tbn(const tuple_def* tpl_d, const void* tupl, positional_accessor inline_accessor, tuple_hasher* th, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const void* transaction_id, int* abort_error);

#define hash_text hash_tbn
#define hash_blob hash_tbn
#define hash_numeric hash_tbn

#endif