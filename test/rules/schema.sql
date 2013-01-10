CREATE TABLE test_rules(id serial, dt date, val text);

DO $$
DECLARE
        v_begin date;
        v_end date;
BEGIN
        FOR i IN 0..11 LOOP
                v_begin = '2012-01-01'::date + (i||' months')::interval;
                v_end = date_trunc('month', v_begin + '35 days'::interval) - '1 day'::interval;
                EXECUTE 'CREATE TABLE test_rules_2012_'||trim(to_char(i+1, '00'))||'() INHERITS(test_rules);';
                EXECUTE 'CREATE RULE partition_2012_'||trim(to_char(i+1, '00'))
				        || ' AS ON INSERT TO test_rules WHERE (dt BETWEEN '''||v_begin::text||''' AND '''||v_end::text||''')'
						|| ' DO INSTEAD INSERT INTO test_rules_2012_'||trim(to_char(i+1, '00'))||' VALUES(NEW.*)';
        END LOOP;
END;
$$;

