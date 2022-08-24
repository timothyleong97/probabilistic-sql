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
 * Gate I/O
 ******************************/

// Returns the textual representation of any gate.
PG_FUNCTION_INFO_V1(gate_out);
Datum gate_out(PG_FUNCTION_ARGS)
{
    // Pull out the gate from the arguments
    Gate *gate = (Gate *)PG_GETARG_POINTER(0);

    // Use the helper
    char *result = _stringify_gate(gate);

    PG_RETURN_CSTRING(result);
}

// Creates a gate.
PG_FUNCTION_INFO_V1(gate_in);
Datum gate_in(PG_FUNCTION_ARGS)
{
    // Pull out the distribution and parameters
    char *literal = PG_GETARG_CSTRING(0);

    // Prepare a few variables for any possible pack of params
    double x, y;

    // See if the distribution matches any of the ones we can recognise
    if (sscanf(literal, "gaussian(%lf, %lf)", &x, &y) == 2)
    {
        Gate *gate = new_gaussian(x, y);
        PG_RETURN_POINTER(gate);
    }
    else if (sscanf(literal, "poisson(%lf)", &x) == 1)
    {
        Gate *gate = new_poisson(x);
        PG_RETURN_POINTER(gate);
    }
    else
    {
        // Catchall for unrecognised literals
        ereport(ERROR,
                errcode(ERRCODE_CASE_NOT_FOUND),
                errmsg("Cannot recognise the type of gate"));
    }
}

/*******************************
 * Gate Composition
 ******************************/
// Adds two gates representing probability variables.
PG_FUNCTION_INFO_V1(add_prob_var);
Datum add_prob_var(PG_FUNCTION_ARGS)
{
    // Read in arguments
    Gate *first_operand = (Gate *)PG_GETARG_POINTER(0);
    Gate *second_operand = (Gate *)PG_GETARG_POINTER(1);

    // Return result
    Gate *new_gate = add(first_operand, second_operand);
    PG_RETURN_POINTER(new_gate);
}