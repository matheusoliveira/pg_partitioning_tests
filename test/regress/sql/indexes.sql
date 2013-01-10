-- Let's create indexes on the child tables, and check if the trigger respect them

CREATE INDEX idx_test_cc_2012_01 ON test_cc_2012_01 (dt);
CREATE INDEX idx_test_cc_2012_02 ON test_cc_2012_02 (dt);
CREATE INDEX idx_test_cc_2012_03 ON test_cc_2012_03 (dt);
CREATE INDEX idx_test_cc_2012_04 ON test_cc_2012_04 (dt);
CREATE INDEX idx_test_cc_2012_05 ON test_cc_2012_05 (dt);
CREATE INDEX idx_test_cc_2012_06 ON test_cc_2012_06 (dt);
CREATE INDEX idx_test_cc_2012_07 ON test_cc_2012_07 (dt);
CREATE INDEX idx_test_cc_2012_08 ON test_cc_2012_08 (dt);
CREATE INDEX idx_test_cc_2012_09 ON test_cc_2012_09 (dt);
CREATE INDEX idx_test_cc_2012_10 ON test_cc_2012_10 (dt);
CREATE INDEX idx_test_cc_2012_11 ON test_cc_2012_11 (dt);
CREATE INDEX idx_test_cc_2012_12 ON test_cc_2012_12 (dt);

-- The insert test again
\i sql/insert_data_2.sql

-- The selects at verify test should now use the indexes (index-only scans)
SET enable_seqscan TO off;

\i sql/verify.sql

