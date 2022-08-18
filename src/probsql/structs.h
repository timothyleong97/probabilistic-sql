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
typedef struct
{
    // Whether this condiiton is an equality or some inequality
    condition_type condition_type;
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
#endif