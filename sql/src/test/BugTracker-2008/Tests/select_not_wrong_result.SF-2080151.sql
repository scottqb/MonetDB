CREATE TABLE A (
a varchar(10)
);
INSERT INTO A VALUES ('a1');
INSERT INTO A VALUES ('a2');

CREATE TABLE B (
a varchar(10)
);
INSERT INTO B VALUES ('a1');
INSERT INTO B VALUES ('a2');
INSERT INTO B VALUES ('a3');

SELECT COUNT(*)
FROM A
WHERE NOT (A.a =
(SELECT B.a
FROM B
WHERE a > 'a4' ));


drop table B;
drop table A;
