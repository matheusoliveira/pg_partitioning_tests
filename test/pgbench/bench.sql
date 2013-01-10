INSERT INTO :tablename (dt, val) VALUES('2012-01-01'::date + (floor(random()*365)::text||' days')::interval, md5(random()::text));

