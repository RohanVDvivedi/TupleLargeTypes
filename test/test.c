#include<stdio.h>
#include<stdlib.h>

#include<blob_large.h>
#include<text_large.h>

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

char* test_data = "Rohan is a good boy,"
					"Rohan is probably a bad boy,"
					"Common sense is not all that common,"
					"Does Rohan have any?"
					"Not if this code functions as required,"
					"What else can I write here?"
					"No one cares what would be written here."
					"I will probably change it to lorem ipsum once this project goes big enough";

tuple_def tpl_d;
tuple_def* get_tuple_def()
{
	return NULL;
}

int main()
{
	return 0;
}