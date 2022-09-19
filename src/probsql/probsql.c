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
#include <parser/parse_oper.h>
#include <catalog/namespace.h>

#include <string.h>

PG_MODULE_MAGIC;

/*******************************
 * Gate I/O
 ******************************/

// SQL gate functions
static Oid and_gate = InvalidOid;
static Oid or_gate = InvalidOid;
static Oid negate_condition_oid = InvalidOid;
static Oid eq = InvalidOid;
static Oid leq = InvalidOid;
static Oid lt = InvalidOid;
static Oid geq = InvalidOid;
static Oid gt = InvalidOid;
static Oid neq = InvalidOid;

// SQL gate operators
static Oid less_than_comparator = InvalidOid;
static Oid less_than_or_equal_comparator = InvalidOid;
static Oid more_than_comparator = InvalidOid;
static Oid more_than_or_equal_comparator = InvalidOid;
static Oid equal_comparator = InvalidOid;
static Oid not_equal_comparator = InvalidOid;

// SQL gate type oid
static Oid gate_oid = InvalidOid;

// The default gate for a new table
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

static Oid get_func_oid(char *s)
{
    FuncCandidateList fcl = FuncnameGetCandidates(
        list_make1(makeString(s)),
        -1,
        NIL,
        false,
        false,
        false,
        false);

    if (fcl == NULL || fcl->oid == InvalidOid)
    {
        ereport(ERROR, errmsg("%s func_oid not found", s));
    }

    return fcl->oid;
}

static Oid find_oper_oid(char *op_name, bool isPrefix)
{
    Oid operatorObjectId = OpernameGetOprid(list_make1(makeString(op_name)), isPrefix ? InvalidOid : gate_oid, gate_oid);
    if (operatorObjectId == InvalidOid)
    {
        ereport(ERROR, errmsg("%s oper_oid not found", op_name));
    }
    return operatorObjectId;
}

PG_FUNCTION_INFO_V1(get_oids);
Datum get_oids(PG_FUNCTION_ARGS)
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

    // Get all function OIDs (names come from the SQL wrapper)
    and_gate = get_func_oid("and_gate");
    or_gate = get_func_oid("or_gate");
    negate_condition_oid = get_func_oid("negate_condition");
    eq = get_func_oid("equal_to");
    leq = get_func_oid("less_than_or_equal");
    lt = get_func_oid("less_than");
    geq = get_func_oid("more_than_or_equal");
    gt = get_func_oid("more_than");
    neq = get_func_oid("not_equal_to");

    // Get all operator OIDs
    less_than_comparator = find_oper_oid("<", false);
    less_than_or_equal_comparator = find_oper_oid("<=", false);
    more_than_comparator = find_oper_oid(">", false);
    more_than_or_equal_comparator = find_oper_oid(">=", false);
    equal_comparator = find_oper_oid("=", false);
    not_equal_comparator = find_oper_oid("<>", false);

    PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(get_true_gate);
Datum get_true_gate(PG_FUNCTION_ARGS)
{
    PG_RETURN_POINTER(true_gate);
}

/**************************
 * Extension hook methods
 **************************/

/* Saved hook values in case of unload */
static planner_hook_type prev_planner = NULL;
static ProcessUtility_hook_type prev_ProcessUtility = NULL;

// The condition column name
static char *PROBSQL_CONDITION = "cond";

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
                ColumnDef *column = makeColumnDef(PROBSQL_CONDITION, gate_oid, -1, 0);
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
                elog_node_display(INFO, "Unrecognised node tag in handle_create_table_with_gate", expr, true);
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
        ereport(INFO, errmsg("Cannot support functional predicates because of the possibility of side-effects"));
    }
    else if (IsA(node, OpExpr))
    {
        // E.g. +, -
        OpExpr *opExpr = castNode(OpExpr, node);
        const int numargs = list_length(opExpr->args);

        HasGateWalkerContext *firstChildContext = new_gate_walker_context();
        Node *firstarg = get_leftop(opExpr);
        has_gate_in_condition_walker(firstarg, firstChildContext);

        HasGateWalkerContext *secondChildContext = new_gate_walker_context();
        Node *secondarg = get_rightop(opExpr);
        has_gate_in_condition_walker(secondarg, secondChildContext);

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
    else if (IsA(node, BoolExpr))
    {

        BoolExpr *boolExpr = castNode(BoolExpr, node);
        const int numargs = list_length(boolExpr->args);

        if (numargs == 1)
        {
            // E.g. NOT(x1 < x2).
            HasGateWalkerContext *childContext = new_gate_walker_context();
            Node *child = linitial(boolExpr->args);
            has_gate_in_condition_walker(child, childContext);

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
            Node *firstarg = linitial(boolExpr->args);
            has_gate_in_condition_walker(firstarg, firstChildContext);

            HasGateWalkerContext *secondChildContext = new_gate_walker_context();
            Node *secondarg = lsecond(boolExpr->args);
            has_gate_in_condition_walker(secondarg, secondChildContext);

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
                // Clone this boolexpr, and set the child arguments to those returned by the childContexts
                // for copy-on-write + "path compression"
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
        elog_node_display(INFO, "Detected unfamiliar node in where clause tree", node, true);
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
    HasGateWalkerContext *context = new_gate_walker_context();

    // Check if query is a SELECT on some table
    if (!query->commandType == CMD_SELECT || !query->rtable)
    {
        return context;
    }

    // Check the WHERE clause for a gate
    FromExpr *jointree = query->jointree;

    if (!jointree)
    {
        return context;
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
    has_gate_in_condition_walker(quals, context);
    // elog_node_display(INFO, "Gate walk context", context->node, true);
    return context;
}

/*
    For a quals tree like

          AND
        /     \
        <     ==
       / \    / \
      x1 x2  x3 40

    where the AND, < and == are SQL operators, I will construct a gate where the
    SQL operators are replaced with the gate functional counterparts, so that the final node specifies a gate and not a boolean.

    Note that AND gates and OR gates can have multiple args, while my probabilistic operators only take 2.
*/
static Node *convert_sql_ops_to_gate_funcs(Node *node)
{
    if (node == NULL)
    {
        return node;
    }

    if (IsA(node, OpExpr))
    {
        // Check the operator type
        OpExpr *original_expression = castNode(OpExpr, node);

        // Convert the argument nodes first
        ListCell *lc;
        int list_len = list_length(original_expression->args);
        foreach (lc, original_expression->args)
        {
            lfirst(lc) = convert_sql_ops_to_gate_funcs(lfirst(lc));
        }

        FuncExpr *final_expression;

        /*
            Because in the WHERE clause, comparators must produce booleans, I have made the < operator (as in g1 < g2) a comparator,
            but in fact I need the < operator to turn its arguments into a gate.

            Hence, the conditional clauses below take the boolean operators, which are the comparators, and transforms them into a FuncExpr
            where the comparator operator is replaced with the corresponding < operator which creates a condition gate.

            The plus/minus/times/divide operators take gates and return gates, so they don't require processing.
        */
        if (original_expression->opno == less_than_comparator)
        {
            final_expression = makeFuncExpr(lt, gate_oid, original_expression->args, InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);
        }
        else if (original_expression->opno == less_than_or_equal_comparator)
        {
            final_expression = makeFuncExpr(leq, gate_oid, original_expression->args, InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);
        }
        else if (original_expression->opno == more_than_comparator)
        {
            final_expression = makeFuncExpr(gt, gate_oid, original_expression->args, InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);
        }
        else if (original_expression->opno == more_than_or_equal_comparator)
        {
            final_expression = makeFuncExpr(geq, gate_oid, original_expression->args, InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);
        }
        else if (original_expression->opno == equal_comparator)
        {
            final_expression = makeFuncExpr(eq, gate_oid, original_expression->args, InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);
        }
        else if (original_expression->opno == not_equal_comparator)
        {
            final_expression = makeFuncExpr(neq, gate_oid, original_expression->args, InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);
        }
        else
        {
            return node;
        }

        return castNode(Node, final_expression);
    }
    else if (IsA(node, BoolExpr))
    {
        // Convert the arguments first
        BoolExpr *boolExpr = castNode(BoolExpr, node);
        ListCell *lc;
        int list_len = list_length(boolExpr->args);
        foreach (lc, boolExpr->args)
        {
            lfirst(lc) = convert_sql_ops_to_gate_funcs(lfirst(lc));
        }

        // Convert the boolean comparator to its gate equivalent.
        FuncExpr *result;
        if (boolExpr->boolop == AND_EXPR)
        {
            // Convert the first two args
            result = makeFuncExpr(and_gate, gate_oid, list_make2(linitial(boolExpr->args), lsecond(boolExpr->args)), InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);

            // Then do the rest
            for_each_from(lc, boolExpr->args, 2)
            {
                result = makeFuncExpr(and_gate, gate_oid, list_make2(result, lfirst(lc)), InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);
            }
        }
        else if (boolExpr->boolop == OR_EXPR)
        {
            // Convert the first two args
            result = makeFuncExpr(or_gate, gate_oid, list_make2(linitial(boolExpr->args), lsecond(boolExpr->args)), InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);

            // Then do the rest
            for_each_from(lc, boolExpr->args, 2)
            {
                result = makeFuncExpr(or_gate, gate_oid, list_make2(result, lfirst(lc)), InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);
            }
        }
        else if (boolExpr->boolop == NOT_EXPR)
        {
            result = makeFuncExpr(negate_condition_oid, gate_oid, boolExpr->args, InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);
        }

        return castNode(Node, result);
    }
    else if (IsA(node, Var) || IsA(node, Const) || IsA(node, CoerceViaIO))
    {
        // No operator to convert.
        return node;
    }
    else
    {
        // Catchall for unknown node types
        elog_node_display(INFO, "Unrecognised node type in convert node to gate", node, true);
        return node;
    }
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
*/
static void construct_condition_column(Query *query, Node *node)
{
    /*  Get the list of all cond columns in the search query's range tables.
        Because I want to AND these columns in the end, I want to keep the reference to the
        cond columns in a list of Vars.
    */
    List *condition_columns = NIL;
    List *tables_inspected = NIL; // Stores relids of tables
    List *rtable = query->rtable;
    ListCell *lc;

    int table_index = 0; // the index of the rtable which we need to give to Var
    foreach (lc, rtable)
    {
        ++table_index;
        RangeTblEntry *rte = castNode(RangeTblEntry, lfirst(lc));

        /*
            This logic is brittle in the sense because it assumes that a column named cond is
            the column for the probabilistic condition.
        */
        Alias *alias = rte->eref; // Tells you what the colnames are

        ListCell *lc2;
        int column_index = 0; // the index of the column which we need to give to Var
        foreach (lc2, alias->colnames)
        {
            ++column_index;
            if (strcmp(strVal(lfirst(lc2)), PROBSQL_CONDITION) == 0)
            {
                // If you have seen this relid before, that means this query
                // is a self-join. Don't double add this condition column.
                ListCell *lc3;
                bool skipTable = false;
                foreach (lc3, tables_inspected)
                {
                    Oid prev_seen_relid = lfirst_oid(lc3);

                    if (prev_seen_relid == rte->relid)
                    {
                        skipTable = true;
                        break;
                    }
                }

                if (skipTable)
                {
                    break;
                }

                // Add a Var referencing this column to condition_columns
                // and add the relid of this table to tables_inspected.
                // Subqueries not supported.
                Var *var = makeVar(table_index, column_index, gate_oid, -1, InvalidOid, 0);
                condition_columns = lappend(condition_columns, var);
                tables_inspected = lappend_oid(tables_inspected, rte->relid);
            }
        }
    }

    // elog_node_display(INFO, "Condition columns", condition_columns, true);

    // Exclude those cond columns in the target list
    List *cells_to_delete = NIL;
    foreach (lc, query->targetList)
    {
        TargetEntry *targetEntry = castNode(TargetEntry, lfirst(lc));
        /*
            This logic is brittle also, because it assumes the condition name is never used by the user elsewhere,
            and that the user did not purposely select cond as ALIAS_NAME.
            Sample query: select first.x+second.x, second.cond from a as first, a as second where first.x < 1::gate;

        */
        if (strcmp(targetEntry->resname, PROBSQL_CONDITION) == 0)
        {
            // Mark this list cell for deletion from the target list
            // Implementation detail: cells_to_delete is a List of ListCells pointing to original ListCells in
            // the targetList.
            cells_to_delete = lappend(cells_to_delete, lc);
        }
    }

    // elog_node_display(INFO, "Cells to delete", cells_to_delete, true);

    int targetEntries_deleted = 0;
    foreach (lc, cells_to_delete)
    {
        query->targetList = list_delete_cell(query->targetList, lfirst(lc));
    }

    // Fix the resno of the target entries
    int resno = 1;
    foreach (lc, query->targetList)
    {
        TargetEntry *targetEntry = lfirst(lc);
        targetEntry->resno = resno;
        ++resno;
    }

    // elog_node_display(INFO, "Final targetList", query->targetList, true);

    /*
        Create an Expr to ANDs all those cond columns together with the given node
        If the where clause is empty, then the node passed in will be null, so there's no new condition to AND.
        However, you still have to AND the conditions in the tables, if there are any.
        It's possible that both the WHERE clause is empty and there are no tables with conds, in which case nothing has to be done.

        As a first step, I find out how many variables we have.

        It could be zero, if this select does not involve any table with gates.
        It could just be one, if I am projecting from a p-table. I just put that one condition column back into the target list.
        It could be two or more, if I have a probabilistic condition, and I have condition columns in the tables.
    */
    int num_cond_gates = 0;
    if (node != NULL)
    {
        ++num_cond_gates;
    }
    num_cond_gates += list_length(condition_columns);

    // Case 1: There are no condition variables.
    if (num_cond_gates == 0)
    {
        // No need to rewrite the query.
        return;
    }
    else if (num_cond_gates == 1)
    {
        // Case 2: There is only one condition variable.
        // If no condition columns in rtables - ok.
        // If there is no WHERE clause and just one condition column, use node to hold that condition column.
        if (node == NULL)
        {
            node = linitial(condition_columns);
        }
    }
    else
    {
        // Case 3: You have at least one condition column, and possibly node is non-null.
        // They need to be conjoined into one gate.
        if (node != NULL)
        {
            condition_columns = lappend(condition_columns, node); // Conds are Vars, node is some OpExpr or BoolExpr
        }

        // Perform a AND, with SQL's boolean AND as the conjoiner. I do the operator replacement in one shot later.
        Expr *result_condition_expr = makeBoolExpr(
            AND_EXPR,
            condition_columns,
            -1);
        node = castNode(Node, result_condition_expr);
    }

    // elog_node_display(INFO, "Initial condition column", node, true);

    // Replace the SQL boolean comparators with the probabilistic counterparts
    node = convert_sql_ops_to_gate_funcs(node);

    // elog_node_display(INFO, "Final condition column", node, true);

    // Put node in the target list.
    TargetEntry *targetEntry = makeTargetEntry(
        castNode(Expr, node), // Gate conditions can only be Exprs
        1 + list_length(query->targetList),
        PROBSQL_CONDITION,
        false);
    query->targetList = lappend(query->targetList, targetEntry);

    elog_node_display(INFO, "Final query", query, true);
}

// Forward declaration of this extension's planner
static PlannedStmt *prob_planner(Query *parse, const char *query_string, int cursorOptions, ParamListInfo boundParams)
{
    if (parse->commandType == CMD_SELECT && parse->rtable)
    {
        elog_node_display(INFO, "Initial query", parse, true);

        // Strip out all the deterministic checks in the WHERE clause, if any.
        HasGateWalkerContext *selectContext = handle_select_from_table_with_gate_in_condition(parse);

        // Rewrite the query to generate the new condition column using the context
        construct_condition_column(parse, selectContext->node); // impl detail: selectContext is always non-null.
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
