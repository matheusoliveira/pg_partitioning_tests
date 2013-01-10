CREATE FUNCTION partition_insert_trigger() RETURNS trigger
    LANGUAGE c STRICT
    AS 'partition_insert_trigger', 'partition_insert_trigger'
	SET DateStyle TO 'ISO,MDY';

-- IMPORTANT: There is a limitation that makes the function depending on DateSytle=ISO

