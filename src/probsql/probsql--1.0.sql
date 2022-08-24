-- Forward declaration of the gate type
CREATE TYPE gate;

-- Declare SQL wrappers around C functions
CREATE FUNCTION gate_in(cstring)
    RETURNS gate 
    AS 'MODULE_PATHNAME', 'new_gate' 
    LANGUAGE C IMMUTABLE STRICT;


CREATE FUNCTION gate_out(gate)
    RETURNS cstring
    AS 'MODULE_PATHNAME', 'stringify_gate'
    LANGUAGE C IMMUTABLE STRICT;

-- Define the gate type
CREATE TYPE gate (
    internallength = 32,
    input = gate_in,
    output = gate_out
);
