#ifndef STRUCT_H
#define STRUCT_H
#include "enums.h"
/************************************************
 * Parameters for distributions
 ************************************************/
typedef struct
{
    double mean;
    double stddev;
} gaussian_parameters;

typedef struct
{
    double lambda;
} poisson_parameters;

// A constant can be represented by a histogram with one bin
// but since it's used commonly enough, I create one struct for it.
typedef struct
{
    double number;
} single_number;

typedef union
{
    gaussian_parameters gaussian_parameters;
    poisson_parameters poisson_parameters;
} base_variable_parameters;
/************************************************
 * Types of gates
 ************************************************/

// Represents a base distribution.
typedef struct
{
    // What type of distribution this base_var has
    distribution_type distribution_type;

    // The parameters for this base variable,
    // depending on what type of distribution it has.
    base_variable_parameters base_variable_parameters;
} base_variable;

// Represents a composition of distributions.
typedef struct
{
    // Whether this composition is a +-*/
    probabilistic_composition opr;

} comp_variable;

// Represents a boolean condition.
// Gates need to be pointers to avoid circular definition.
typedef struct
{
    // Whether this condiiton is an equality or some inequality
    condition_type condition_type;
    // The left operand in this condition, e.g. x in x == y.
    struct Gate *left_gate;
    // The right operand in this condition, e.g. y in x > y.
    struct Gate *right_gate;
} condition;

typedef union
{
    // The information corresponding to a base random variable.
    base_variable base_variable;

    // The information corresponding to a composite variable.
    comp_variable comp_variable;

    // The information corresponding to a condition.
    condition condition;
} gate_info;

/************************************************
 * Outer shell of a gate
 ************************************************/

// The atomic unit of a boolean circuit
typedef struct Gate
{
    // We use this to discriminate the union below.
    gate_type gate_type;

    // The information pertaining to this gate,
    // depending on what gate type it has.
    gate_info gate_info;
} Gate;

#endif