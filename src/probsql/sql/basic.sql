CREATE TABLE test(id SERIAL, gate GATE);
INSERT INTO test(gate) VALUES ('gaussian(1.0, 2.0)'), ('poisson(3.0)');
SELECT * FROM test;
SELECT 'gaussian(1.0, 2.0)'::gate + 'poisson(3.0)'::gate AS comp_var;
SELECT 'gaussian(1.0, 2.0)'::gate - 'poisson(3.0)'::gate AS comp_var;
SELECT 'gaussian(1.0, 2.0)'::gate * 'poisson(3.0)'::gate AS comp_var;
SELECT 'gaussian(1.0, 2.0)'::gate / 'poisson(3.0)'::gate AS comp_var;
