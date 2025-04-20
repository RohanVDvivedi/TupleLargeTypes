#ifndef JSONB_PARSER_H
#define JSONB_PARSER_H

#include<cutlery/stream.h>
#include<tuplelargetypes/jsonb_node.h>

// only on error returns in NULL
// you need to call finalize_jsonb on the return value of the below function before serializing it
jsonb_node* parse_jsonb(stream* rs);

#endif