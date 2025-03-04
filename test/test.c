#include<stdio.h>
#include<stdlib.h>

#include<blob_large.h>
#include<text_large.h>
#include<text_blob_read_iterator.h>
#include<text_blob_write_iterator.h>

#define USE_TEXT
//#define USE_BLOB

#define USE_BASE
//#define USE_NESTED

#ifdef USE_BASE
	#define ACCS SELF
#else
	#define ACCS STATIC_POSITION(0)
#endif

#define PREFIX_SIZE 15

#define READ_CHUNK_SIZE 5
//#define READ_CHUNK_SIZE 100

#define WRITE_CHUNK_SIZE 5
//#define WRITE_CHUNK_SIZE 100

#define PAGE_SIZE 256
#define PAGE_ID_WIDTH 3

tuple_def tpl_d;
tuple_def* get_tuple_def()
{
	return NULL;
}

int main()
{
	return 0;
}