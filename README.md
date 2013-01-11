Introduction
============

This is just an experimental code intended to compare methods to do bulk
inserts on partitioned tables in PostgreSQL.

For now, the codes used on this project *should not be used in production*.

Partition insert trigger using C
================================

The `src/partition_insert_trigger.c` contains the, probably, most fast
method. It is based on the source code of PostgreSQL 9.2, and will probably
not work on different versions (perhaps not even newer versions).

There are another, more safe, method at `src/spi/partition_insert_trigger_spi.c`,
which implements the same function, but it creates an INSERT statement to
redirect the row to the right child table.

Comparison method results
=========================

Here I've some methods, and got the following results, based on a bulk insert
of 1 million rows, into a table with 12 partitions.

The results above are based on a three run of the `inserts_test.sql` files
located on the subdirectories of `test/`. All the test runs are in
`test/results.txt`. The average runs are the following:

- Inserting 1G rows without partition: 17663.377 ms

- Inserting 1G rows with PL/pgSQL and `EXECUTE`: 66901.375 ms

  Tests at `test/pgsql/`

- Inserting 1G rows with C and `SPI_(prepare/keepplan/execute_plan)`: 36558.564 ms

  Tests at `test/spi/`

  SPEEDUP compared with PL/pgSQL: 1.829

- Inserting 1G rows with C and `heap_insert` (with `insert_partition_trigger`): 31896.098 ms

  Tests at `test/c/`

  SPEEDUP compared with PL/pgSQL: 1.970.

  SPEEDUP compared with C and `SPI_execute_with_args`: 1.146.

- Inserting 1G rows with RULES: 318446.826 ms

  Tests at `test/rules/`

There is also some `pgbench` runs, that shows that for 1M rows, inserting
on a not partitioned table took 5 minutes and 21 seconds, while inserting
on a partitioned (12 partitions) took 5 minutes and 26 seconds, using the
`insert_partition_trigger`.

Authors
=======

Developed by Matheus de Oliveira `matheus.oliveira (at) dextra (dot) com (dot) br`.
And it is supported by [Dextra Sistemas](http://www.dextra.com.br/).

