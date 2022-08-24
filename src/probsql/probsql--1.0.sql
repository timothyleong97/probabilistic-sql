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

-- Define functions for the now-defined gate
CREATE FUNCTION add_prob_var(gate, gate)
    RETURNS gate
    AS 'MODULE_PATHNAME', 'add_prob_var'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR + (
    leftarg = gate,
    rightarg = gate,
    function = add_prob_var
);

CREATE FUNCTION sub_prob_var(gate, gate)
    RETURNS gate
    AS 'MODULE_PATHNAME', 'sub_prob_var'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR - (
    leftarg = gate,
    rightarg = gate,
    function = sub_prob_var
);

CREATE FUNCTION times_prob_var(gate, gate)
    RETURNS gate
    AS 'MODULE_PATHNAME', 'times_prob_var'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR * (
    leftarg = gate,
    rightarg = gate,
    function = times_prob_var
);

CREATE FUNCTION div_prob_var(gate, gate)
    RETURNS gate
    AS 'MODULE_PATHNAME', 'div_prob_var'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR / (
    leftarg = gate,
    rightarg = gate,
    function = div_prob_var
);