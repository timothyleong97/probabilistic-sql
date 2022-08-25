// Methods for gate operations
#ifndef GATE_H
#define GATE_H
#include "enums.h"
#include "structs.h"
#include "postgres.h"

/**
 * @brief Create a new Gate representing a Gaussian distribution
 *
 * @param mean The mean of the distribution
 * @param stddev The standard deviation
 * @return Gate* The object representing this distribution
 */
Gate *new_gaussian(double mean, double stddev)
{
    Gate *result = (Gate *)palloc(sizeof(Gate));
    result->gate_type = BASE_VARIABLE;
    result->gate_info.base_variable.distribution_type = GAUSSIAN;
    gaussian_parameters params = {mean, stddev};
    result->gate_info.base_variable.base_variable_parameters.gaussian_parameters = params;
    return result;
}

/**
 * @brief Create a new Gate representing a Poisson distribution
 *
 * @param lambda The mean of the distribution
 * @return Gate* The object representing this distribution
 */
Gate *new_poisson(double lambda)
{
    Gate *result = (Gate *)palloc(sizeof(Gate));
    result->gate_type = BASE_VARIABLE;
    result->gate_info.base_variable.distribution_type = POISSON;
    result->gate_info.base_variable.base_variable_parameters.poisson_parameters.lambda = lambda;
    return result;
}

/**
 * @brief Create a new Gate representing a constant
 *
 * @param constant The numeric constant
 * @return Gate* A zero variance Gaussian variable whose mean equals the constant.
 */
Gate *constant(double constant)
{
    return new_gaussian(constant, 0);
}

/**
 * @brief Performs some arithmetic operation on two gates, i.e. X + Y
 *
 * @param gate1 The first probability gate
 * @param gate2 The second probability gate
 * @param opr The operator to apply
 * @return Gate* The new gate
 */
Gate *combine_prob_gates(Gate *gate1, Gate *gate2, probabilistic_composition opr)
{
    // Check that both gates represent probability variables.
    if (gate1->gate_type == CONDITION || gate2->gate_type == CONDITION)
    {
        ereport(ERROR,
                errcode(ERRCODE_WRONG_OBJECT_TYPE),
                errmsg("Detected condition gate instead of prob gate"));
    }

    // Create the result gate
    Gate *result = (Gate *)palloc(sizeof(Gate));
    result->gate_type = COMPOSITE_VARIABLE;
    result->gate_info.comp_variable.opr = opr;
    result->gate_info.comp_variable.left_gate = gate1;
    result->gate_info.comp_variable.right_gate = gate2;

    return result;
}

/**
 * @brief Create a condition from two gates. Only comparator conditions are allowed, like
 * X < Y or X = Y.
 *
 * @param gate1 The first probability gate
 * @param gate2 The second probability gate
 * @param condition The condition between the two
 * @return Gate* The new gate
 */
Gate *create_condition_from_prob_gates(Gate *gate1, Gate *gate2, condition_type opr)
{
    // Check that gates are probability gates
    if (gate1->gate_type == CONDITION || gate2->gate_type == CONDITION)
    {
        ereport(ERROR,
                errcode(ERRCODE_WRONG_OBJECT_TYPE),
                errmsg("Detected condition gate instead of prob gate"));
    }

    // Check that the condition is a comparator condition
    if (!condition_is_comparator(opr))
    {
        ereport(ERROR,
                errcode(ERRCODE_WRONG_OBJECT_TYPE),
                errmsg("Detected boolean condition instead of comparator condition"));
    }

    // Create the result gate
    Gate *result = (Gate *)palloc(sizeof(Gate));
    result->gate_type = CONDITION;
    condition cdn = {opr, gate1, gate2};
    result->gate_info.condition = cdn;

    return result;
}

/**
 * @brief Create a condition through two existing conditions. Only boolean conditions are
 * allowed, like X AND Y or X OR Y.
 *
 * @param gate1 The first condition gate
 * @param gate2 The second condition gate
 * @param condition The operator (AND/OR) to combine the two conditions
 * @return Gate* The new gate
 */
Gate *combine_two_conditions(Gate *gate1, Gate *gate2, condition_type opr)
{
    // Check that the gates are probability gates
    if (gate1->gate_type != CONDITION || gate2->gate_type != CONDITION)
    {
        ereport(ERROR,
                errcode(ERRCODE_WRONG_OBJECT_TYPE),
                errmsg("Detected prob gate instead of condition gate"));
    }

    // Check that the condition is a boolean condition
    if (condition_is_comparator(opr))
    {
        ereport(ERROR,
                errcode(ERRCODE_WRONG_OBJECT_TYPE),
                errmsg("Detected comparator condition instead of boolean condition"));
    }

    // Create the result gate
    Gate *result = (Gate *)palloc(sizeof(Gate));
    result->gate_type = CONDITION;
    condition cdn = {opr, gate1, gate2};
    result->gate_info.condition = cdn;

    return result;
}
#endif