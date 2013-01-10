\timing
DO $$
BEGIN
	FOR i IN 1..1000000 LOOP
		INSERT INTO test_pgsql(dt, val)
		VALUES('2012-01-01'::date + (floor(random()*365)::text||' days')::interval, md5(random()::text));
	END LOOP;
END;
$$;

