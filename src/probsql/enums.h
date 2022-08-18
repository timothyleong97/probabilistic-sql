#ifndef ENUMS_H
#define ENUMS_H
// Represents the type of base distributions that exist.
typedef enum
{
    GAUSSIAN,
    POISSON
} distribution_type;

// Represents the type of composition of distributions
// of a gate.
typedef enum
{
    PLUS,
    MINUS,
    TIMES,
    DIVIDE
} probabilistic_composition;

// The types of condition gates that exist.
typedef enum
{
    LESS_THAN_OR_EQUAL,
    LESS_THAN,
    MORE_THAN_OR_EQUAL,
    MORE_THAN,
    EQUAL_TO,
    AND,
    OR
} condition_type;

// The type of gates that exist.
typedef enum
{
    BASE_VARIABLE,
    COMPOSITE_VARIABLE,
    CONDITION
} gate_type;
#endif