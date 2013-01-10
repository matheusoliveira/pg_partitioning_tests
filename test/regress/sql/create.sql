CREATE TABLE test_cc (
    id serial,
    dt date
);

CREATE FUNCTION partition_insert_trigger() RETURNS trigger
    LANGUAGE c STRICT
    AS 'partition_insert_trigger', 'partition_insert_trigger'
	SET DateStyle TO 'ISO,MDY';

