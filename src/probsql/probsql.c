#include "probsql.h"
#include "stringify.h"

#include <postgres.h>
#include <fmgr.h>

#include <lib/stringinfo.h>
#include <libpq/pqformat.h>

#include <string.h>

// Returns the textual representation of a gate.
PG_FUNCTION_INFO_V1(stringify_gate);
Datum stringify_gate(PG_FUNCTION_ARGS)
{
    // Pull out the gate from the arguments
    Gate *gate = (Gate *)PG_GETARG_POINTER(0);

    // Use the helper
    char *result = _stringify_gate(gate);

    PG_RETURN_CSTRING(result);
}

PG_MODULE_MAGIC;