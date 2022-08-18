#ifndef STRUCT_H
#define STRUCT_H
#include "enums.h"

// Represents a base distribution.
typedef struct
{

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
} package;
#endif