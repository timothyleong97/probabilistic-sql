-- Forward declaration of the gate type
CREATE TYPE gate;

-- Declare SQL wrappers around C functions
CREATE FUNCTION gate_in(cstring)
    RETURNS gate 
    AS 'MODULE_PATHNAME', 'gate_in' 
    LANGUAGE C IMMUTABLE STRICT;


CREATE FUNCTION gate_out(gate)
    RETURNS cstring
    AS 'MODULE_PATHNAME', 'gate_out'
    LANGUAGE C IMMUTABLE STRICT;

-- Define the gate type
CREATE TYPE gate (
    internallength = 32,
    input = gate_in,
    output = gate_out
);

-- Provide a way for constants to get coerced into gates.
CREATE CAST (numeric AS gate)
    WITH INOUT
    AS IMPLICIT;

CREATE CAST (int AS gate)
    WITH INOUT
    AS IMPLICIT;


-- Get the Oid of the gate type for internal use.
CREATE FUNCTION get_gate_oid()
    RETURNS void 
    AS 'MODULE_PATHNAME', 'get_gate_oid'
    LANGUAGE C IMMUTABLE STRICT;

SELECT get_gate_oid();

-- Define arithmetic functions for the now-defined gate
CREATE FUNCTION arithmetic_var(gate, gate, cstring)
    RETURNS gate
    AS 'MODULE_PATHNAME', 'arithmetic_var'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION add_prob_var(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT arithmetic_var(g1, g2, 'PLUS')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR + (
    leftarg = gate,
    rightarg = gate,
    function = add_prob_var
);

CREATE FUNCTION sub_prob_var(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT arithmetic_var(g1, g2, 'MINUS')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR - (
    leftarg = gate,
    rightarg = gate,
    function = sub_prob_var
);

CREATE FUNCTION times_prob_var(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT arithmetic_var(g1, g2, 'TIMES')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR * (
    leftarg = gate,
    rightarg = gate,
    function = times_prob_var
);

CREATE FUNCTION div_prob_var(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT arithmetic_var(g1, g2, 'DIVIDE')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR / (
    leftarg = gate,
    rightarg = gate,
    function = div_prob_var
);

-- Define conditioning functions for the now-defined gate
CREATE FUNCTION return_true(gate, gate) -- for WHERE CLAUSE
    RETURNS boolean AS
    $$
        SELECT true;
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE FUNCTION create_condition_from_var_and_var(gate, gate, cstring)
    RETURNS gate
    AS 'MODULE_PATHNAME', 'create_condition_from_var_and_var'
    LANGUAGE C IMMUTABLE STRICT;


CREATE FUNCTION less_than_or_equal(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT create_condition_from_var_and_var(g1, g2, 'LESS_THAN_OR_EQUAL')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR <= (
    leftarg = gate,
    rightarg = gate,
    function = return_true
);

CREATE FUNCTION less_than(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT create_condition_from_var_and_var(g1, g2, 'LESS_THAN')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR < (
    leftarg = gate,
    rightarg = gate,
    function = return_true
);

CREATE FUNCTION more_than_or_equal(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT create_condition_from_var_and_var(g1, g2, 'MORE_THAN_OR_EQUAL')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR >= (
    leftarg = gate,
    rightarg = gate,
    function = return_true
);

CREATE FUNCTION more_than(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT create_condition_from_var_and_var(g1, g2, 'MORE_THAN')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR > (
    leftarg = gate,
    rightarg = gate,
    function = return_true
);

CREATE FUNCTION equal_to(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT create_condition_from_var_and_var(g1, g2, 'EQUAL_TO')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR = (
    leftarg = gate,
    rightarg = gate,
    function = return_true
);

CREATE FUNCTION not_equal_to(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT create_condition_from_var_and_var(g1, g2, 'NOT_EQUAL_TO')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR != (
    leftarg = gate,
    rightarg = gate,
    function = return_true
);

CREATE FUNCTION combine_condition(gate, gate, cstring)
    RETURNS gate
    AS 'MODULE_PATHNAME', 'combine_condition'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION and_gate(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT combine_condition(g1, g2, 'AND')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR && (
    leftarg = gate,
    rightarg = gate,
    function = and_gate
);

CREATE FUNCTION or_gate(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT combine_condition(g1, g2, 'OR')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR || (
    leftarg = gate,
    rightarg = gate,
    function = or_gate
);

CREATE FUNCTION negate_condition(gate)
    RETURNS gate 
    AS 'MODULE_PATHNAME', 'negate_condition_gate'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR ! (
    rightarg = gate,
    function = negate_condition
);

-- Functions for creating/removing a condition column
CREATE FUNCTION add_condition(_tbl regclass)
RETURNS void AS
$$
BEGIN
    EXECUTE format('ALTER TABLE %I ADD COLUMN cond gate DEFAULT (1::gate = 1::gate)', _tbl);
END
$$ LANGUAGE plpgsql;

CREATE FUNCTION drop_condition(_tbl regclass)
RETURNS void AS
$$
BEGIN
    EXECUTE format('ALTER TABLE %I DROP COLUMN cond', _tbl);
END
$$ LANGUAGE plpgsql;

-- A function to retrieve the true gate.
CREATE FUNCTION get_true_gate()
RETURNS gate AS 'MODULE_PATHNAME', 'get_true_gate'
LANGUAGE C IMMUTABLE STRICT;
