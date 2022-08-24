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
 * @brief Performs some arithmetic operation on two gates
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
#endif