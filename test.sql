drop table a;
drop extension probsql;
create extension probsql;
create table a(x gate);
insert into a values (1), (2), (3), (4);
select prob_sum(x) from a;
-- create table a(x int);
-- insert into a values (1), (2), (3), (4);
-- select sum_num(x) from a;