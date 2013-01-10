CREATE TABLE test_cc(id serial, dt date, val text);

DO $$
DECLARE
        v_begin date;
        v_end date;
BEGIN
        FOR i IN 0..11 LOOP
                v_begin = '2012-01-01'::date + (i||' months')::interval;
                v_end = date_trunc('month', v_begin + '35 days'::interval) - '1 day'::interval;
                EXECUTE 'CREATE TABLE test_cc_2012_'||trim(to_char(i+1, '00'))||'() INHERITS(test_cc);';
                EXECUTE 'ALTER TABLE test_cc_2012_'||trim(to_char(i+1, '00'))||
                        ' ADD CONSTRAINT chk_partition CHECK (dt BETWEEN '''||v_begin::text||''' AND '''||v_end::text||''');';
        END LOOP;
END;
$$;

CREATE OR REPLACE FUNCTION partition_insert_trigger()
RETURNS trigger
LANGUAGE C
VOLATILE STRICT
AS 'partition_insert_trigger','partition_insert_trigger'
SET DateStyle TO 'ISO';

CREATE TRIGGER partition_insert_trigger
BEFORE INSERT ON test_cc
FOR EACH ROW EXECUTE PROCEDURE partition_insert_trigger('dt');

