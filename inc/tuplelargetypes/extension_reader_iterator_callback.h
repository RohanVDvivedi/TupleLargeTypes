#ifndef EXTENSION_READER_ITERATOR_CALLBACK_H
#define EXTENSION_READER_ITERATOR_CALLBACK_H

typedef struct extension_reader_iterator_callback extension_reader_iterator_callback;
struct extension_reader_iterator_callback
{
	void* context1;
	void* context2;
	void (*extension_blob_read_begin_event)(extension_reader_iterator_callback* callback, const datum* uval, const data_type_info* dti, const page_access_methods* pam_p);
	void (*extension_blob_read_ended_event)(extension_reader_iterator_callback* callback, const datum* uval, const data_type_info* dti, const page_access_methods* pam_p);
};

#endif