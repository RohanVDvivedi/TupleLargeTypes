#ifndef JSONB_SERIALIZE_H
#define JSONB_SERIALIZE_H

#include<tuplelargetypes/jsonb_writer_interface.h>
#include<tuplelargetypes/jsonb_node.h>

// only a successfully finalized jsonb_node must be serialized, else it will result in undefined behaviour
// returns 1 for success, OR 0 in an error, consult your jsonb_writer_interface upon an error
int jsonb_serialize(const jsonb_writer_interface* jwi_p, const jsonb_node* node_p);

#endif