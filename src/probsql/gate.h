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
    result->gate_info.base_variable.base_variable_parameters.gaussian_parameters.mean = mean;
    result->gate_info.base_variable.base_variable_parameters.gaussian_parameters.stddev = stddev;
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
#endif