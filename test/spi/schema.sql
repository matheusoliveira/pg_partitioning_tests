CREATE TABLE test_spi(id serial, dt date, val text);

DO $$
DECLARE
        v_begin date;
        v_end date;
BEGIN
        FOR i IN 0..11 LOOP
                v_begin = '2012-01-01'::date + (i||' months')::interval;
                v_end = date_trunc('month', v_begin + '35 days'::interval) - '1 day'::interval;
                EXECUTE 'CREATE TABLE test_spi_2012_'||trim(to_char(i+1, '00'))||'() INHERITS(test_spi);';
                EXECUTE 'ALTER TABLE test_spi_2012_'||trim(to_char(i+1, '00'))||
                        ' ADD CONSTRAINT chk_partition CHECK (dt BETWEEN '''||v_begin::text||''' AND '''||v_end::text||''');';
        END LOOP;
END;
$$;

CREATE OR REPLACE FUNCTION partition_insert_trigger_spi()
RETURNS trigger
LANGUAGE C
VOLATILE STRICT
AS 'partition_insert_trigger_spi','partition_insert_trigger_spi'
SET DateStyle TO 'ISO';

CREATE TRIGGER partition_insert_trigger_spi
BEFORE INSERT ON test_spi
FOR EACH ROW EXECUTE PROCEDURE partition_insert_trigger_spi('dt');

