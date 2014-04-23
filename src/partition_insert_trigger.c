/*-------------------------------------------------------------------------
 *
 * partition_insert_trigger.c
 *	  A function to redirect insertions from parent table to child
 *	  partitions.
 *	  
 *	  IMPORTANT: This is just an experimental code
 *
 * Author: Matheus de Oliveira <matioli (dot) matheus (at) gmail.com>
 *    based on PostgreSQL source code and Emmanuel Cecchet's code at
 *    http://archives.postgresql.org/pgsql-hackers/2008-12/msg01221.php
 *-------------------------------------------------------------------------
 */
#include <postgres.h>
#include <math.h>
#include <commands/trigger.h>
#include <executor/executor.h>
#include <executor/spi.h>
#include <utils/rel.h>
#include <utils/date.h>
#include <utils/datetime.h>
#include <utils/timestamp.h>
#include <executor/nodeModifyTable.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#if PG_VERSION_NUM < 90400 && defined(__GNUC__)
#warning "These functions can only work on 9.4+"
#endif

extern Datum partition_insert_trigger(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(partition_insert_trigger);

void InsertIntoTable(const char *table_name, HeapTuple tuple /*, TupleDesc tupdesc*/ )
{
	ResultRelInfo	*resultRelInfo;
	TupleTableSlot	*slot;
	EState 			*estate = CreateExecutorState();
	Relation relation;
	Oid			relation_id;

	/* Lookup the relation */
	relation_id = RelnameGetRelid(table_name);
	if (relation_id == InvalidOid)
		elog(ERROR, "partition_insert_trigger: Invalid child table %s", table_name);
	relation = RelationIdGetRelation(relation_id);
	if (relation == NULL)
		elog(ERROR, "partition_insert_trigger: Failed to locate relation for child table %s", table_name);
	SimpleInsertTuple(relation, tuple);
}

/* ----------------------------------------------------------------
 *		partition_insert_trigger
 *
 *		Trigger to redirect the tuple to a child table, based on the field
 *		name passed by the first argument, which should be a date type.
 *		The partition choice is fixed and goes to the table named as:
 *			Format("%s_%04d_%02d", parenttablename, dt year, dt month)
 *
 * ----------------------------------------------------------------
 */
Datum partition_insert_trigger(PG_FUNCTION_ARGS) {
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	TupleDesc tupdesc;
	HeapTuple rettuple;
	int ret;
	int attrnum;
	char **args;
	DateADT dt;
	int mday, month, year;
	char *relname;
	char child_table_name[NAMEDATALEN];
	bool isnull;

	/* make sure it's called as a before insert trigger */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "insert_partition: not called by trigger manager");

	if (!TRIGGER_FIRED_BEFORE(trigdata->tg_event))
		elog(ERROR, "insert_partition: should be called as BEFORE");

	if (!TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
		elog(ERROR, "insert_partition: should be called for INSERT");

	tupdesc = trigdata->tg_relation->rd_att;
	rettuple = trigdata->tg_trigtuple;
	relname = RelationGetRelationName(trigdata->tg_relation);
	args = trigdata->tg_trigger->tgargs;

	/* get the column that contains the partition key */
	attrnum = SPI_fnumber(tupdesc, args[0]);
	if (attrnum <= 0)
		elog(ERROR, "insert_partition: column \"%s\" not found", args[0]);

	/* get the date value and retrieve year, month and day as integers */
	dt = DatumGetDateADT(SPI_getbinval(rettuple, tupdesc, attrnum, &isnull));
	if (DATE_NOT_FINITE(dt))
		elog(ERROR, "insert_partition: date must be finite");
	if (isnull)
		elog(ERROR, "insert_partition: date cannot be NULL");
	j2date(dt + POSTGRES_EPOCH_JDATE, &year, &month, &mday);

	/* get the table name, based on the date field */
	if (month >= 1 && month <= 12
		&& snprintf(child_table_name, sizeof(child_table_name), "%s_%04d_%02d", relname, year, month) < 0
	)
		elog(ERROR, "insert_partition: %d-%d invalid year-month", year, month);

	/* insert the tuple on the child table */
	InsertIntoTable(child_table_name, rettuple /*, tupdesc */);

	/* return null, as it has been inserted on a child table */
	return PointerGetDatum(NULL);
}

