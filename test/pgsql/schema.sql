CREATE TABLE test_pgsql(id serial, dt date, val text);

DO $$
DECLARE
        v_begin date;
        v_end date;
BEGIN
        FOR i IN 0..11 LOOP
                v_begin = '2012-01-01'::date + (i||' months')::interval;
                v_end = date_trunc('month', v_begin + '35 days'::interval) - '1 day'::interval;
                EXECUTE 'CREATE TABLE test_pgsql_2012_'||trim(to_char(i+1, '00'))||'() INHERITS(test_pgsql);';
                EXECUTE 'ALTER TABLE test_pgsql_2012_'||trim(to_char(i+1, '00'))||
                        ' ADD CONSTRAINT chk_partition CHECK (dt BETWEEN '''||v_begin::text||''' AND '''||v_end::text||''');';
        END LOOP;
END;
$$;

CREATE OR REPLACE FUNCTION test_pgsql_partition()
RETURNS TRIGGER
LANGUAGE plpgsql VOLATILE AS $$
BEGIN
	EXECUTE 'INSERT INTO test_pgsql_'||to_char(new.dt, 'YYYY_MM')||' VALUES(($1).*)'
	USING NEW;
	RETURN NULL;
END;
$$;

CREATE TRIGGER test_pgsql_partition
BEFORE INSERT ON test_pgsql
FOR EACH ROW EXECUTE PROCEDURE test_pgsql_partition();

