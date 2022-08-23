#include "probsql.h"
#include "stringify.h"
#include "gate.h"

#include <postgres.h>
#include <fmgr.h>

#include <lib/stringinfo.h>
#include <libpq/pqformat.h>

#include <string.h>

PG_MODULE_MAGIC;

/*******************************
 * Methods for gate operations
 ******************************/

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

// Creates a base variable gate.
PG_FUNCTION_INFO_V1(base_var);
Datum base_var(PG_FUNCTION_ARGS)
{
    // Pull out the distribution and parameters
    char *literal = PG_GETARG_CSTRING(0);

    // Prepare a few variables for any possible pack of params
    double x, y;

    // See if the distribution matches any of the ones we can recognise
    if (sscanf(literal, "gaussian(%f, %f)", &x, &y) == 2)
    {
        Gate *gate = new_gaussian(x, y);
        PG_RETURN_POINTER(gate);
    }
    else if (sscanf(literal, "poisson(%f)", &x) == 1)
    {
        Gate *gate = new_possion(x);
        PG_RETURN_POINTER(gate);
    }
    else
    {
        // Catchall for unrecognised literals
        ereport(ERROR,
                errcode(ERRCODE_CASE_NOT_FOUND),
                errmsg("Cannot recognise the type of base variable"));
    }
}