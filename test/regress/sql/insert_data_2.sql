DO $$
DECLARE
        v_begin date;
        v_end date;
BEGIN
        FOR i IN 0..11 LOOP
                v_begin = '2012-01-01'::date + (i||' months')::interval;
                v_end = date_trunc('month', v_begin + '1 month'::interval) - '1 day'::interval;
				INSERT INTO test_cc(dt) VALUES(v_begin),(v_begin + interval '10 days'),(v_begin + interval '15 days'),(v_begin + interval '20 days'),(v_begin + interval '25 days'),(v_end);
        END LOOP;
END;
$$;

