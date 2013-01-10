CREATE TABLE test_cc_2012_01 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-01-01'::date) AND (dt <= '2012-01-31'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_02 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-02-01'::date) AND (dt <= '2012-02-29'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_03 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-03-01'::date) AND (dt <= '2012-03-31'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_04 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-04-01'::date) AND (dt <= '2012-04-30'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_05 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-05-01'::date) AND (dt <= '2012-05-31'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_06 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-06-01'::date) AND (dt <= '2012-06-30'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_07 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-07-01'::date) AND (dt <= '2012-07-31'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_08 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-08-01'::date) AND (dt <= '2012-08-31'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_09 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-09-01'::date) AND (dt <= '2012-09-30'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_10 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-10-01'::date) AND (dt <= '2012-10-31'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_11 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-11-01'::date) AND (dt <= '2012-11-30'::date)))
)
INHERITS (test_cc);

CREATE TABLE test_cc_2012_12 (
    CONSTRAINT chk_partition CHECK (((dt >= '2012-12-01'::date) AND (dt <= '2012-12-31'::date)))
)
INHERITS (test_cc);

CREATE TRIGGER partition_insert_trigger
BEFORE INSERT ON test_cc
FOR EACH ROW EXECUTE PROCEDURE partition_insert_trigger('dt');

INSERT INTO test_cc
SELECT * FROM ONLY test_cc;

TRUNCATE ONLY test_cc;

\i sql/verify.sql
\i sql/insert_data_2.sql
\i sql/verify.sql

