#ifndef JSONB_PARSE_H
#define JSONB_PARSE_H

#include<tuplelargetypes/jsonb_node.h>
#include<tuplelargetypes/jsonb_reader_interface.h>

// only an error returns in NULL
jsonb_node* jsonb_parse(jsonb_reader_interface* jri_p);

#endif