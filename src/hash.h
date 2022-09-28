#ifndef HASH_H
#define HASH_H
#include <postgres.h>
#include <uuid.h>

/*
 * Hashtable key that defines the identity of a hashtable entry.  We separate
 * gates by uuid.
 */
typedef struct probsqlHashKey {
    pg_uuid_t gate_id;
    
};
#endif