#include<text_blob_write_iterator.h>

text_blob_write_iterator* get_new_text_blob_write_iterator(void* tupl, tuple_def* tpl_d, positional_accessor inline_accessor, uint32_t bytes_to_be_written_to_prefix, const worm_tuple_defs* wtd_p, const page_access_methods* pam_p, const page_modification_methods* pmm_p);

void delete_text_blob_write_iterator(text_blob_write_iterator* tbwi, const void* transaction_id, int* abort_error);

uint32_t append_to_text_blob(text_blob_write_iterator* tbwi, const char* data, uint32_t data_size, const void* transaction_id, int* abort_error);