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
    DIVIDE,
    MAX,
    MIN,
    COUNT,
    SUM
} probabilistic_composition;

bool is_aggregate_comp(probabilistic_composition pc)
{
    return pc == MAX || pc == MIN || pc == COUNT || pc == SUM;
}

// The types of condition gates that exist.
typedef enum
{
    LESS_THAN_OR_EQUAL,
    LESS_THAN,
    MORE_THAN_OR_EQUAL,
    MORE_THAN,
    EQUAL_TO,
    NOT_EQUAL_TO,
    AND,
    OR
} condition_type;

bool condition_is_comparator(condition_type cdn)
{
    return (cdn != AND) && (cdn != OR);
}

// The type of gates that exist.
typedef enum
{
    BASE_VARIABLE,
    COMPOSITE_VARIABLE,
    CONDITION,
    PLACEHOLDER_TRUE // Used for the trivial condition
} gate_type;

bool is_prob_type(gate_type g)
{
    return g == BASE_VARIABLE || g == COMPOSITE_VARIABLE;
}
#endif