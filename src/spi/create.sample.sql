CREATE FUNCTION partition_insert_trigger_spi() RETURNS trigger
    LANGUAGE c STRICT
    AS 'partition_insert_trigger_spi', 'partition_insert_trigger_spi'
	SET DateStyle TO 'ISO,MDY';

-- IMPORTANT: There is a limitation that makes the function depending on DateSytle=ISO

