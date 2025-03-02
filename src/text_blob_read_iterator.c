#include<text_blob_read_iterator.h>

text_blob_read_iterator* get_new_text_blob_read_iterator(void* tupl, tuple_def* tpl_d, positional_accessor inline_data_accessor, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p);

void delete_text_blob_read_iterator(text_blob_read_iterator* tbri, const void* transaction_id, int* abort_error);

text_blob_read_iterator* clone_text_blob_read_iterator(text_blob_read_iterator* tbri, const void* transaction_id, int* abort_error);

uint32_t read_from_text_blob(text_blob_read_iterator* tbri, void* data, uint32_t data_size, const void* transaction_id, int* abort_error);