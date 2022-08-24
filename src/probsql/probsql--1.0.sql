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
