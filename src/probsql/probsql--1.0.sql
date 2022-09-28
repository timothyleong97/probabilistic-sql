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

CREATE FUNCTION unary_minus(g gate)
    RETURNS gate AS
    $$
        SELECT sub_prob_var(0::gate, g);
    $$ 
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR - (
    rightarg = gate,
    function = unary_minus
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

-- CREATE FUNCTION max_prob_var(g1 gate, g2 gate)
--     RETURNS gate AS
--     $$
--         SELECT arithmetic_var(g1, g2, 'MAX');
--     $$
--     LANGUAGE SQL IMMUTABLE STRICT;

-- CREATE AGGREGATE max (gate)
-- (
--     sfunc = max_prob_var,
--     stype = gate
-- );

-- CREATE FUNCTION min_prob_var(g1 gate, g2 gate)
--     RETURNS gate AS
--     $$
--         SELECT arithmetic_var(g1, g2, 'MIN');
--     $$
--     LANGUAGE SQL IMMUTABLE STRICT;

-- CREATE AGGREGATE min (gate)
-- (
--     sfunc = min_prob_var,
--     stype = gate
-- );

-- CREATE FUNCTION count_prob_var(g1 gate, g2 gate)
--     RETURNS gate AS
--     $$
--         SELECT arithmetic_var(g1, g2, 'COUNT');
--     $$
--     LANGUAGE SQL IMMUTABLE STRICT;

-- CREATE AGGREGATE count (gate)
-- (
--     sfunc = count_prob_var,
--     stype = gate
-- );

CREATE FUNCTION sum_prob_var(_state gate, _value gate)
    RETURNS gate AS
    $$
        SELECT arithmetic_var(_state, _value, 'SUM');
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE AGGREGATE prob_sum (gate)
(
    sfunc = sum_prob_var,
    stype = gate,
    initcond = '0'
);


CREATE FUNCTION sum_nums(_state numeric, _value numeric)
    RETURNS numeric AS
    $$
        SELECT _state + 2 * _value;
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE AGGREGATE sum_num (numeric) (
    sfunc = sum_nums,
    stype = numeric
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


CREATE FUNCTION or_gate(g1 gate, g2 gate)
    RETURNS gate AS
    $$
        SELECT combine_condition(g1, g2, 'OR')
    $$
    LANGUAGE SQL IMMUTABLE STRICT;


CREATE FUNCTION negate_condition(gate)
    RETURNS gate 
    AS 'MODULE_PATHNAME', 'negate_condition_gate'
    LANGUAGE C IMMUTABLE STRICT;

-- Operator class for Postgres to implement DISTINCT
-- Ref: https://stackoverflow.com/questions/34971181/creating-custom-equality-operator-for-postgresql-type-point-for-distinct-cal
CREATE FUNCTION gate_compare(gate, gate)
RETURNS integer LANGUAGE SQL IMMUTABLE AS 
$$
    SELECT 0;
$$;

CREATE OPERATOR CLASS gate_ops
    DEFAULT FOR TYPE gate USING btree AS
        operator 1 <,
        operator 2 <=,
        operator 3 =,
        operator 4 >=,
        operator 5 >,
        function 1 gate_compare(gate, gate);


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


-- Get the Oids of user-defined types/functions for internal use.
CREATE FUNCTION get_oids()
    RETURNS void 
    AS 'MODULE_PATHNAME', 'get_oids'
    LANGUAGE C IMMUTABLE STRICT;

SELECT get_oids();