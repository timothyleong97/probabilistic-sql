CREATE TABLE test(id SERIAL, gate GATE);
INSERT INTO test(gate) VALUES ('gaussian(1.0, 2.0)');
SELECT * FROM test;