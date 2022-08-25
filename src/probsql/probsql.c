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
    else if (sscanf(literal, "%lf", &x) == 1)
    {
        Gate *gate = constant(x);
        PG_RETURN_POINTER(gate);
    }
    else
    {
        // Catchall for unrecognised literals
        ereport(ERROR,
                errcode(ERRCODE_CASE_NOT_FOUND),
                errmsg("Cannot recognise the type of gate: %s", literal));
    }
}

/*******************************
 * Gate Composition
 ******************************/
// Perform arithmetic on two probability gates.
PG_FUNCTION_INFO_V1(arithmetic_var);
Datum arithmetic_var(PG_FUNCTION_ARGS)
{
    // Read in arguments
    Gate *first_operand = (Gate *)PG_GETARG_POINTER(0);
    Gate *second_operand = (Gate *)PG_GETARG_POINTER(1);
    char *opr = PG_GETARG_CSTRING(2);

    // Determine the type of composition
    probabilistic_composition comp;

    if (strcmp(opr, "PLUS") == 0)
    {
        comp = PLUS;
    }
    else if (strcmp(opr, "MINUS") == 0)
    {
        comp = MINUS;
    }
    else if (strcmp(opr, "TIMES") == 0)
    {
        comp = TIMES;
    }
    else if (strcmp(opr, "DIVIDE") == 0)
    {
        comp = DIVIDE;
    }
    else
    {
        // Catchall for unrecognised operator codes
        ereport(ERROR,
                errcode(ERRCODE_CASE_NOT_FOUND),
                errmsg("Cannot recognise the type of operator"));
    }

    // Create the new gate
    Gate *new_gate = combine_prob_gates(first_operand, second_operand, comp);
    PG_RETURN_POINTER(new_gate);
}


// PG_FUNCTION_INFO_V1(create_condition_from_var_and_const);
// Datum create_condition_from_var_and_const(PG_FUNCTION_ARGS)
// {
//     // Read in arguments
//     Gate *first_gate = (Gate *)PG_GETARG_POINTER(0);
//     double number = PG_GETARG_FLOAT8(1);
//     char *comparator = PG_GETARG_CSTRING(2);

//     // Determine the type of comparator
//     condition_type cond;
//     if (strcmp(comparator, "LESS_THAN_OR_EQUAL") == 0)
//     {
//         cond = LESS_THAN_OR_EQUAL;
//     }
//     else if (strcmp(comparator, "LESS_THAN") == 0)
//     {
//         cond = LESS_THAN;
//     }
//     else if (strcmp(comparator, "MORE_THAN_OR_EQUAL") == 0)
//     {
//         cond = MORE_THAN_OR_EQUAL;
//     }
//     else if (strcmp(comparator, "MORE_THAN") == 0)
//     {
//         cond = MORE_THAN;
//     }
//     else if (strcmp(comparator, "EQUAL_TO") == 0)
//     {
//         cond = EQUAL_TO;
//     }
//     else
//     {
//         // Catchall for unrecognised comparator codes
//         ereport(ERROR,
//                 errcode(ERRCODE_CASE_NOT_FOUND),
//                 errmsg("Cannot recognise the type of comparator"));
//     }

//     // Create a gate for the constant
//     Gate *gate_for_constant = constant(number);

//     // Return result
//     Gate *new_gate = create_condition_from_prob_gates(first_gate, gate_for_constant, cond);
//     PG_RETURN_POINTER(new_gate);
// }

PG_FUNCTION_INFO_V1(create_condition_from_var_and_var);
Datum create_condition_from_var_and_var(PG_FUNCTION_ARGS)
{
    // Read in arguments
    Gate *first_gate = (Gate *)PG_GETARG_POINTER(0);
    Gate *second_gate = (Gate *)PG_GETARG_POINTER(1);
    char *comparator = PG_GETARG_CSTRING(2);

    // Determine the type of comparator
    condition_type cond;
    if (strcmp(comparator, "LESS_THAN_OR_EQUAL") == 0)
    {
        cond = LESS_THAN_OR_EQUAL;
    }
    else if (strcmp(comparator, "LESS_THAN") == 0)
    {
        cond = LESS_THAN;
    }
    else if (strcmp(comparator, "MORE_THAN_OR_EQUAL") == 0)
    {
        cond = MORE_THAN_OR_EQUAL;
    }
    else if (strcmp(comparator, "MORE_THAN") == 0)
    {
        cond = MORE_THAN;
    }
    else if (strcmp(comparator, "EQUAL_TO") == 0)
    {
        cond = EQUAL_TO;
    }
    else if (strcmp(comparator, "NOT_EQUAL_TO") == 0)
    {
        cond = NOT_EQUAL_TO;
    }
    else
    {
        // Catchall for unrecognised comparator codes
        ereport(ERROR,
                errcode(ERRCODE_CASE_NOT_FOUND),
                errmsg("Cannot recognise the type of comparator"));
    }
    
    // Return result
    Gate *new_gate = create_condition_from_prob_gates(first_gate, second_gate, cond);
    PG_RETURN_POINTER(new_gate);
}

// PG_FUNCTION_INFO_V1(and_condition);
// Datum and_condition(PG_FUNCTION_ARGS)
// {
//     // Read in arguments
//     Gate *first_gate = (Gate *)PG_GETARG_POINTER(0);
//     Gate *second_gate = (Gate *)PG_GETARG_POINTER(1);

//     // Return result
//     Gate *new_gate = combine_two_conditions(first_gate, second_gate, AND);
//     PG_RETURN_POINTER(new_gate);
// }

// PG_FUNCTION_INFO_V1(or_condition);
// Datum or_condition(PG_FUNCTION_ARGS)
// {
//     // Read in arguments
//     Gate *first_gate = (Gate *)PG_GETARG_POINTER(0);
//     Gate *second_gate = (Gate *)PG_GETARG_POINTER(1);

//     // Return result
//     Gate *new_gate = combine_two_conditions(first_gate, second_gate, OR);
//     PG_RETURN_POINTER(new_gate);
// }