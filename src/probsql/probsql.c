#include "probsql.h"
#include "stringify.h"
#include "gate.h"

#include <postgres.h>
#include <fmgr.h>
#include <optimizer/planner.h>
#include <tcop/utility.h>
#include <lib/stringinfo.h>
#include <libpq/pqformat.h>
#include <nodes/print.h>
#include <parser/parse_type.h>
#include <nodes/makefuncs.h>
#include <nodes/nodeFuncs.h>
#include <nodes/value.h>
#include <catalog/heap.h>
#include <parser/parser.h>

#include <string.h>

PG_MODULE_MAGIC;

/*******************************
 * Gate I/O
 ******************************/

static Oid gate_oid = InvalidOid;
static Gate *true_gate = NULL;

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

PG_FUNCTION_INFO_V1(combine_condition);
Datum combine_condition(PG_FUNCTION_ARGS)
{
    // Read in arguments
    Gate *first_gate = (Gate *)PG_GETARG_POINTER(0);
    Gate *second_gate = (Gate *)PG_GETARG_POINTER(1);
    char *combiner = PG_GETARG_CSTRING(2);

    // Determine the combiner in use
    condition_type cond;
    if (strcmp(combiner, "AND") == 0)
    {
        cond = AND;
    }
    else if (strcmp(combiner, "OR") == 0)
    {
        cond = OR;
    }
    else
    {
        // Catchall for unrecognised comparator codes
        ereport(ERROR,
                errcode(ERRCODE_CASE_NOT_FOUND),
                errmsg("Cannot recognise the type of combiner"));
    }
    // Return result
    Gate *new_gate = combine_two_conditions(first_gate, second_gate, AND);
    PG_RETURN_POINTER(new_gate);
}

PG_FUNCTION_INFO_V1(negate_condition_gate);
Datum negate_condition_gate(PG_FUNCTION_ARGS)
{
    // Read in argument
    Gate *gate = (Gate *)PG_GETARG_POINTER(0);

    // Perform negation
    gate = negate_condition(gate);
    PG_RETURN_POINTER(gate);
}

PG_FUNCTION_INFO_V1(get_gate_oid);
Datum get_gate_oid(PG_FUNCTION_ARGS)
{
    // Create a node that holds the type name of a gate
    Value *value = makeString("gate");

    // Convert this node to a list
    TypeName *typename = makeTypeNameFromNameList(list_make1(value));

    // Set the global value
    gate_oid = typenameTypeId(NULL, typename);
    ereport(INFO, errmsg("Gate OID: %u", gate_oid));

    // Also create the default gate for true
    true_gate = (Gate *)malloc(sizeof(Gate));
    true_gate->gate_type = PLACEHOLDER_TRUE;
}

PG_FUNCTION_INFO_V1(get_true_gate);
Datum get_true_gate(PG_FUNCTION_ARGS)
{
    ereport(INFO, errmsg("Type = %u", true_gate->gate_type));
    PG_RETURN_POINTER(true_gate);
}

/**************************
 * Extension hook methods
 **************************/

/* Saved hook values in case of unload */
static planner_hook_type prev_planner = NULL;
static ProcessUtility_hook_type prev_ProcessUtility = NULL;

/*
    Looks out for CREATE TABLE [AS].
    SELECT INTO will be rewritten into CREATE TABLE AS (see docs for CreateTableAsStmt)
*/
static void handle_create_table_with_gate(PlannedStmt *query)
{
    // Implementation detail: CREATE TABLE is a utility statement
    if (query->commandType != CMD_UTILITY)
        return;

    // Check the node tag of the utility statement
    Node *utility_stmt = query->utilityStmt;
    NodeTag tag = nodeTag(utility_stmt);

    if (tag != T_CreateTableAsStmt && tag != T_CreateStmt)
        return;

    // Get the actual form of the statement
    elog_node_display(INFO, "PlannedStmt inside handle_create_table_with_gate", query, true);

    // Examine the attribute types
    // Ref: https://doxygen.postgresql.org/explain_8c.html#a640ae0e1984b7e39c4348f1db5717af9
    if (tag == T_CreateStmt)
    {
        CreateStmt *stmt = castNode(CreateStmt, utility_stmt);
        ListCell *lc;

        // Ref: https://doxygen.postgresql.org/tablecmds_8c_source.html#l00888
        foreach (lc, stmt->tableElts)
        {
            ColumnDef *colDef = lfirst(lc);
            TypeName *typename = colDef->typeName;
            ListCell *name = list_head(typename->names);

            // Implementation detail: The type name for `gate` is just 1 word, gate
            // with no schema qualifier unlike ("pg_catalog", "int4").
            // Ref: https://doxygen.postgresql.org/parse__type_8c_source.html#l00439
            if (strcmp(strVal(lfirst(name)), "gate") == 0)
            {
                // Insert the condition column as a new ColumnDef
                // and make TRUE the default value for the condition column
                // Ref: https://doxygen.postgresql.org/parse__utilcmd_8c_source.html#l00627
                // and https://doxygen.postgresql.org/parse__expr_8c_source.html#l00094
                // and https://doxygen.postgresql.org/test__rls__hooks_8c_source.html#l00045
                ColumnDef *column = makeColumnDef("cond", gate_oid, -1, 0);
                FuncCall *funccallnode = makeFuncCall(list_make1(makeString("get_true_gate")), NIL, COERCE_EXPLICIT_CALL, -1);
                Constraint *constraint = makeNode(Constraint);
                constraint->contype = CONSTR_DEFAULT;
                constraint->location = -1;
                constraint->raw_expr = (Node *)funccallnode;
                constraint->cooked_expr = NULL;
                column->constraints = lappend(column->constraints, constraint);

                // Add this column to the table
                stmt->tableElts = lappend(stmt->tableElts, column);
                elog_node_display(INFO, "Final create statement", stmt, true);
                return; // Unneeded, but speeds up grokking
            }
        }
    }
    else if (tag == T_CreateTableAsStmt)
    {
        CreateTableAsStmt *stmt = castNode(CreateTableAsStmt, utility_stmt); // elog_node_display does not work for this
        Query *query = castNode(Query, stmt->query);                         // The SELECT statement that populates the table
        List *targetList = query->targetList;

        elog_node_display(INFO, "query", query, true);
        elog_node_display(INFO, "into clause", stmt->into, true);

        ListCell *lc;
        foreach (lc, targetList)
        {
            TargetEntry *entry = castNode(TargetEntry, lfirst(lc));
            if (entry->resjunk)
                continue; // Not going to be in the final attribute list

            elog_node_display(INFO, "target entry", entry, true);

            // The underlying result could have been from a table, or something like
            // CREATE TABLE tbl AS 2::gate, where a literal was coerced into a gate.
            Expr *expr = entry->expr;
            if (IsA(expr, CoerceViaIO))
            {
                // The args don't matter because that's the literal you are coercing from.
                // You want to examine the result type because that's what you coerce the literal
                // into.
                CoerceViaIO *c = castNode(CoerceViaIO, expr);
                if (c->resulttype == gate_oid)
                {
                    ereport(ERROR, errmsg("Currently not supported"));
                }
            }
            else if (IsA(expr, Var))
            {
                Var *v = castNode(Var, expr);
                if (v->vartype == gate_oid)
                {
                    ereport(ERROR, errmsg("Currently not supported"));
                }
            }
            else if (IsA(expr, Const))
            {
                Const *c = castNode(Const, expr);
                if (c->consttype == gate_oid)
                {
                    ereport(ERROR, errmsg("Currently not supported"));
                }
            }
            else
            {
                // There could be some other Node types that I'm not aware of. Log it:
                ereport(ERROR, errmsg("Unrecognised node tag in handle_create_table_with_gate: %u", nodeTag(expr)));
            }
        }
    }
}

// I use this struct to check how to construct the condition column of a query result.
// The node inside this context needs to match the node that this walker is currently working on.
// For e.g., For a boolexpr with two opexprs as its operands, I will use two separate context objects to
// walk through those opexprs, then based on whether they are null or not, I decide what to put into the context of the current
// function call.
typedef struct
{
    Node *node;
} HasGateWalkerContext;

void set_context_to_null(HasGateWalkerContext *context)
{
    context->node = NULL;
}

HasGateWalkerContext *new_gate_walker_context()
{
    HasGateWalkerContext *context = (HasGateWalkerContext *)palloc(sizeof(HasGateWalkerContext));
    set_context_to_null(context);
    return context;
}

// We are walking inside the quals, checking the arguments for any gate-gate or gate-literal comparison.
static bool has_gate_in_condition_walker(Node *node, void *context)
{
    if (node == NULL)
    {
        return false;
    }

    // This context cannot be null.
    HasGateWalkerContext *currContext = (HasGateWalkerContext *)context;

    if (IsA(node, FuncExpr))
    {
        ereport(ERROR, errmsg("Cannot support functional predicates because of the possibility of side-effects"));
    }
    else if (IsA(node, OpExpr))
    {
        // E.g. +, -.
        // Get the number of arguments (1 or 2)
        OpExpr *opExpr = castNode(OpExpr, node);
        const int numargs = list_length(opExpr->args);
        if (numargs == 1)
        {
            // Unexpected
            ereport(ERROR, errmsg("1 arg OpExpr not supported. If negating variable, e.g. -x1, use 0 - x1."));
        }
        else if (numargs == 2)
        {
            HasGateWalkerContext *firstChildContext = new_gate_walker_context();
            Node *firstarg = get_leftop(opExpr);
            expression_tree_walker(firstarg, has_gate_in_condition_walker, firstChildContext);

            HasGateWalkerContext *secondChildContext = new_gate_walker_context();
            Node *secondarg = get_rightop(opExpr);
            expression_tree_walker(secondarg, has_gate_in_condition_walker, secondChildContext);

            /*
                For op exprs, like + or -, if at least one argument has a gate, we need to keep this whole expression, e.g.
                x1 + 2.
            */
            if (firstChildContext->node != NULL || secondChildContext->node != NULL)
            {
                // Simply put this whole opExpr inside the currContext.
                currContext->node = node;
            }
        }
    }
    else if (IsA(node, BoolExpr))
    {

        BoolExpr *boolExpr = castNode(BoolExpr, node);
        const int numargs = list_length(boolExpr->args);

        if (numargs == 1)
        {
            // E.g. NOT(x1 < x2).
            HasGateWalkerContext *childContext = new_gate_walker_context();
            Node *child = lfirst(list_head(boolExpr->args));
            expression_tree_walker(child, has_gate_in_condition_walker, childContext);

            if (childContext->node != NULL)
            {
                // Then we can simply return this gate.
                currContext->node = node;
            }
        }
        else if (numargs == 2)
        {
            // E.g. AND/OR.
            HasGateWalkerContext *firstChildContext = new_gate_walker_context();
            Node *firstarg = lfirst(list_head(boolExpr->args));
            expression_tree_walker(firstarg, has_gate_in_condition_walker, firstChildContext);

            HasGateWalkerContext *secondChildContext = new_gate_walker_context();
            Node *secondarg = lfirst(list_nth_cell(boolExpr->args, 1));
            expression_tree_walker(secondarg, has_gate_in_condition_walker, secondChildContext);

            /*
                 For an expr like

                 name <> 'Singapore'
                 AND
                 (
                    x1 < x2
                    OR
                    name = 'Malaysia'
                 )

                 where name is a text column, I want to strip off all the deterministic checks.
                 So if both children return nothing, I return nothing;
                 If only one child returns something, I return that child - e.g. in the OR above;
                 If both children return something, I return this node.
            */

            if ((firstChildContext->node == NULL) != (secondChildContext->node == NULL))
            {
                // One is empty and the other isn't, so just directly return the child
                currContext->node = firstChildContext->node == NULL ? secondChildContext->node : firstChildContext->node;
            }

            if (firstChildContext->node != NULL && secondChildContext->node != NULL)
            {
                // Clone this boolexpr, and set the child arguments to those returned by the childContexts because of
                // "path compression"
                BoolExpr *clonedBoolExpr = (BoolExpr *)makeBoolExpr(boolExpr->boolop,
                                                                    list_make2(firstChildContext->node, secondChildContext->node),
                                                                    boolExpr->location);

                currContext->node = castNode(Node, clonedBoolExpr);
            }
        }
    }
    else if (IsA(node, Var))
    {
        Var *var = castNode(Var, node);
        if (var->vartype == gate_oid)
        {
            // If the Var is a gate, we want to keep this node and send it back to the parent through the currContext
            currContext->node = node;
        }
    }
    else if (IsA(node, Const))
    {

        Const *c = castNode(Const, node);
        if (c->consttype == gate_oid)
        {
            // If the Const is a gate, we want to keep this node and send it back to the parent through the currContext
            currContext->node = node;
        }
    }
    else if (IsA(node, CoerceViaIO))
    {
        CoerceViaIO *c = castNode(CoerceViaIO, node);
        if (c->resulttype == gate_oid)
        {
            // If the CVI is a gate, we want to keep this node and send it back to the parent through the currContext
            currContext->node = node;
        }
    }
    else
    {
        // Catchall for currently unsupported nodes, such as MinMaxExpr
        ereport(ERROR, errmsg("Detected unfamiliar node in where clause tree"));
    }

    return false; // Always return false because we need to traverse the whole tree.
}

// I only need to modify the condition column of a result when there is already
// an existing condition on a tuple (implying that this tuple is in a table, which implies
// the existence of the cond column), and
// the selection involves a gate such that the new tuple will need a conjunction or disjunction
// in its condition (implying that one of the selection predicates involves some gate column).
static HasGateWalkerContext *handle_select_from_table_with_gate_in_condition(Query *query)
{

    // Check if query is a SELECT on some table
    if (!query->commandType == CMD_SELECT || !query->rtable)
    {
        return NULL;
    }

    // Check the WHERE clause for a gate
    elog_node_display(INFO, "query", query, true);
    FromExpr *jointree = query->jointree;

    if (!jointree)
    {
        return NULL;
    }

    /*
        The rtable in the Query records which tables are involved in this selection. Each node in the rtable is a
        range table entry that records the name and id of a table. The join tree has two attributes, fromexpr and quals.
        fromexpr contains the fromlist, which is a list of range table references that contain the index numbers (1-indexed)
        of the tables in the rtable that are actually being selected from. quals contains the selection predicates, and is a list
        of opexpr (if you are using oprs like <=>) or funcexpr (if the selection predicate is a function that returns a bool).

        I don't support funcexpr currently because how do you calculate the probability of a blackbox with side-effects? A
        function can modify the table in some way such that the probability calculations change from time to time?
    */
    Node *quals = jointree->quals;

    /*
        When walking through the quals, there is no need to examine comparisons on deterministic columns like name <> 'John'
        into the condition column because Postgres immediately removes tuples that fail these comparisons.

        I only look for the existence of at least 1 condition that involves a comparison with a gate and another gate or literal.
    */
    HasGateWalkerContext *context = new_gate_walker_context();
    has_gate_in_condition_walker(quals, context);
    return context;
}

/*
    This function takes a query node, and an expression tree that tells me how to get the new condition column,
    and I will add a new column def in the result that mirrors this node.

    For e.g., if the node represents the following tree:

          &&
        /    \
        <    ==
       / \   / \
      x1 x2 x3 40

    Then I will add a column definition in the query result as follows:
    // TODO
*/
static void construct_condition_column(Query *query, Node *node)
{
}

// Forward declaration of this extension's planner
static PlannedStmt *prob_planner(Query *parse, const char *query_string, int cursorOptions, ParamListInfo boundParams)
{
    /*
        If this is a SELECT whose condition involves a gate, return the same tuples, but with the SELECT condition `and`-ed
        to the tuples.
    */
    HasGateWalkerContext *selectContext = handle_select_from_table_with_gate_in_condition(parse);
    if (selectContext != NULL && selectContext->node != NULL)
    {
        // Rewrite the query to generate the new condition column using the context
    }

    // Let the previous planner (if it exists) or the standard planner run
    if (prev_planner)
    {
        return prev_planner(parse, query_string, cursorOptions, boundParams);
    }
    else
    {
        return standard_planner(parse, query_string, cursorOptions, boundParams);
    }
}

// Hook for CREATE TABLE and ALTER TABLE
static void probsql_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
                                   bool readOnlyTree,
                                   ProcessUtilityContext context, ParamListInfo params,
                                   QueryEnvironment *queryEnv,
                                   DestReceiver *dest, QueryCompletion *qc)
{
    /*
        If this is a CREATE TABLE, and the attributes contain a GATE, insert a condition attribute also of type GATE
        where each tuple's condition is set to TRUE.

        // Use a manual cond inserter and remover as an escape hatch
    */
    handle_create_table_with_gate(pstmt);

    // Let the previous utility processor (if it exists) or the standard utility processor run
    if (prev_ProcessUtility)
    {
        prev_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    }
    else
    {
        standard_ProcessUtility(pstmt, queryString, readOnlyTree, context, params, queryEnv, dest, qc);
    }
}

void _PG_init(void)
{
    // Capture the existing planner
    prev_planner = planner_hook;

    // Replace existing planner with extension's planner
    planner_hook = prob_planner;

    // Capture the existing ProcessUtility_hook
    prev_ProcessUtility = ProcessUtility_hook;

    // Replace existing ProcessUtility_hook with extension's utility processor
    ProcessUtility_hook = probsql_ProcessUtility;
}

void _PG_fini(void)
{
    // Replace the old planner
    planner_hook = prev_planner;

    // Replace the old utility processor
    ProcessUtility_hook = prev_ProcessUtility;

    if (true_gate)
    {
        free(true_gate);
        true_gate = NULL;
    }
}
