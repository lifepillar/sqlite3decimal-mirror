-- Create a test table with data
create table t1(a blob, b blob, c blob);
insert into t1 values (dec(1), dec(0), dec(0));
insert into t1 values (dec(2), dec(0), dec(1));
insert into t1 values (dec(3), dec(1), dec(0));
insert into t1 values (dec(4), dec(1), dec(1));
insert into t1 values (dec(5), null, dec(0));
insert into t1 values (dec(6), null, dec(1));
insert into t1 values (dec(7), null, null);
select decStr(a), decStr(b), decstr(c) from t1;

-- Check to see what CASE does with NULLs in its test expressions
select decStr(a), case when decNotEqual(b, 0) then 1 else 0 end from t1;
select decStr(decAdd(a,10)), case when not decNotEqual(b, 0) then 1 else 0 end from t1;
select decStr(decAdd(a,20)), case when decNotEqual(b, 0) and decNotEqual(c, 0) then 1 else 0 end from t1;
select decStr(decAdd(a,30)), case when not decNotEqual(b, 0) and decNotEqual(c, 0) then 1 else 0 end from t1;
select decStr(decAdd(a,40)), case when decNotEqual(b, 0) or decNotEqual(c, 0) then 1 else 0 end from t1;
select decStr(decAdd(a,50)), case when not decNotEqual(b, 0) or decNotEqual(c, 0) then 1 else 0 end from t1;
select decStr(decAdd(a,60)), case b when c then 1 else 0 end from t1;
select decStr(decAdd(a,70)), case c when b then 1 else 0 end from t1;

-- What happens when you multiply NULL by zero? As is standard in SQL, the result is NULL.
select decStr(decAdd(a,80)), decStr(decMul(b,0)) from t1;
select decStr(decAdd(a,90)), decStr(decMul(b,c)) from t1;

-- What happens to NULL for other operators? If an operand is NULL, the result must be NULL.
select decStr(decAdd(a,100)), decStr(decAdd(b,c)) from t1;

-- Test the treatment of aggregate operators (same as standard SQL aggregate functions)
select count(*), count(b), decStr(decSum(b)), decStr(decAvg(b)), decStr(decMin(b)), decStr(decMax(b)) from t1;

-- Check the behavior of NULLs in WHERE clauses
select decStr(decAdd(a,110)) from t1 where decLt(b, 10);                     -- 1,2,3,4
select decStr(decAdd(a,120)) from t1 where not decGt(b, 10);                 -- 1,2,3,4
select decStr(decAdd(a,130)) from t1 where decLt(b, 10) or decEq(c, 1);      -- 1,2,3,4,6
select decStr(decAdd(a,140)) from t1 where decLt(b, 10) and decEq(c, 1);     -- 2,4
select decStr(decAdd(a,150)) from t1 where not decLt(b, 10) and decEq(c, 1); -- Empty
select decStr(decAdd(a,160)) from t1 where not decEq(c, 1) and decLt(b, 10); -- 1,3

-- Check the behavior of NULLs in a DISTINCT query (usual SQL semantics)
select distinct decStr(b) from t1;

-- Check the behavior of NULLs in a UNION query (usual SQL semantics)
select decStr(b) from t1 union select decStr(b) from t1;

-- Create a new table with a unique column. Check to see if NULLs are
-- considered distinct.
create table t2(a blob, b blob unique);
insert into t2(a,b) values(dec(1),dec(1));
insert into t2(a,b) values(dec(2),null);
insert into t2(a,b) values(dec(3),null); -- OK
select decStr(a), decStr(b) from t2;

drop table t1;
drop table t2;
