#ifndef JSONB_PARSER_H
#define JSONB_PARSER_H

#include<cutlery/stream.h>
#include<tuplelargetypes/jsonb_node.h>

// only on error returns in NULL
jsonb_node* jsonb_parse(stream* rs);

#endif