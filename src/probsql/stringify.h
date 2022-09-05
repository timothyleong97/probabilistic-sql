// Methods for visualising the expressions represented
// by a boolean circuit.
#ifndef STRINGIFY_H
#define STRINGIFY_H
#include "enums.h"
#include "structs.h"

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"

// Forward declaration for stringify_condition
char *_stringify_gate(Gate *gate);

// Returns the textual representation of a base distribution,
// i.e. the name of the distribution and its parameters.
char *stringify_base_variable(base_variable *base_variable)
{
    switch (base_variable->distribution_type)
    {
    case GAUSSIAN:
    {
        gaussian_parameters params = base_variable->base_variable_parameters.gaussian_parameters;
        return psprintf("gaussian(%.2f, %.2f)", params.mean, params.stddev);
    }
    case POISSON:
    {
        poisson_parameters params = base_variable->base_variable_parameters.poisson_parameters;
        return psprintf("poisson(%.2f)", params.lambda);
    }
    default:
        return "UNRECOGNISED_BASE_VARIABLE";
    }
}

// Returns (stringified_left)[ opr ](stringified_right)
char *combine_operands_and_operator(char *stringified_left, char *opr, char *stringified_right)
{
    const int length_of_left_side = strlen(stringified_left);
    const int length_of_opr = strlen(opr);
    const int length_of_right_side = strlen(stringified_right);
    const int length = length_of_left_side +
                       length_of_opr +
                       length_of_right_side + 4; // + 4 for the brackets

    char *arr = (char *)palloc(length + 1); // + 1 for the null terminator

    arr[0] = '(';
    memcpy((void *)arr + 1, (void *)stringified_left, length_of_left_side);
    arr[length_of_left_side + 1] = ')';

    memcpy((void *)arr + length_of_left_side + 2, (void *)opr, length_of_opr);

    arr[length_of_left_side + 2 + length_of_opr] = '(';
    memcpy((void *)arr + length_of_left_side + 3 + length_of_opr, (void *)stringified_right, length_of_right_side);
    arr[length - 1] = ')';
    arr[length] = '\0';

    return arr;
}

char *stringify_condition(condition *condition)
{
    // The operator in the condition
    char *opr;

    switch (condition->condition_type)
    {
    case AND:
        opr = "&&";
        break;
    case OR:
        opr = "||";
        break;
    case LESS_THAN_OR_EQUAL:
        opr = "<=";
        break;
    case LESS_THAN:
        opr = "<";
        break;
    case MORE_THAN_OR_EQUAL:
        opr = ">=";
        break;
    case MORE_THAN:
        opr = ">";
        break;
    case EQUAL_TO:
        opr = "==";
        break;
    case NOT_EQUAL_TO:
        opr = "!=";
        break;
    default:
        opr = "UNRECOGNISED CONDITION";
    }

    /**
     * A condition always takes the form <left gate> <opr> <right gate>
     */
    char *stringified_left = _stringify_gate(condition->left_gate);
    char *stringified_right = _stringify_gate(condition->right_gate);
    return combine_operands_and_operator(stringified_left, opr, stringified_right);
}

char *stringify_composite_variable(comp_variable *comp_variable)
{
    char *opr;
    switch (comp_variable->opr)
    {
    case PLUS:
        opr = "+";
        break;
    case MINUS:
        opr = "-";
        break;
    case TIMES:
        opr = "*";
        break;
    case DIVIDE:
        opr = "/";
        break;
    default:
        opr = "UNKNOWN OPR";
    }

    /**
     *  A composite variable takes the following form: <left gate> <opr> <right_gate>
     */
    char *stringified_left = _stringify_gate(comp_variable->left_gate);
    char *stringified_right = _stringify_gate(comp_variable->right_gate);
    return combine_operands_and_operator(stringified_left, opr, stringified_right);
}

char *_stringify_gate(Gate *gate)
{
    // Depending on the type of gate we have, use the
    // appropriate stringifier
    switch (gate->gate_type)
    {
    case BASE_VARIABLE:
        return stringify_base_variable(&(gate->gate_info.base_variable));
    case COMPOSITE_VARIABLE:
        return stringify_composite_variable(&(gate->gate_info.comp_variable));
    case CONDITION:
        return stringify_condition(&(gate->gate_info.condition));
    case PLACEHOLDER_TRUE:
        return "TRUE";
    default:
        return "UNRECOGNISED_GATE";
    }
}
#endif