#ifndef JSONB_SERIALIZER_H
#define JSONB_SERIALIZER_H

#include<cutlery/stream.h>
#include<tuplelargetypes/jsonb_node.h>

// only a successfully finalized jsonb_node must be serialized, else it will result in undefined behaviour
// returns 1 for success, OR 0 in an error, consult your stream upon an error (when 0 is returned)
int serialize_jsonb(stream* ws, const jsonb_node* node_p);

#endif