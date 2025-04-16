#ifndef JSONB_SERIALIZE_H
#define JSONB_SERIALIZE_H

#include<tuplelargetypes/jsonb_writer_interface.h>

// only a successfully finalized jsonb_node can be serialized
void jsonb_serialize(const jsonb_writer_interface* jwi_p, jsonb_node* node_p);

#endif