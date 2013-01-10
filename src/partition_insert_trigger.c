/*-------------------------------------------------------------------------
 *
 * partition_insert_trigger.c
 *	  A function to redirect insertions from parent table to child
 *	  partitions.
 *	  
 *	  IMPORTANT: This is just an experimental code, and is based on
 *	             PostgreSQL  version 9.2.1, and will probably not work
 *	             on different versions (although not tested)
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

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#if PG_VERSION_NUM < 90200 && defined(__GNUC__)
#warning "These functions has not been tested with versions later than 9.2, and will probably not work! Please, for safety, use partition_insert_trigger_spi instead, or at least run the regress tests!"
#endif

extern Datum partition_insert_trigger(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(partition_insert_trigger);

/* ----------------------------------------------------------------
 *		ExecInsert
 *
 *		For INSERT, we have to insert the tuple into the target relation
 *		and insert appropriate tuples into the index relations.
 *
 *		Adapted from: src/backend/executor/nodeModifyTable.c
 * ----------------------------------------------------------------
 */
static TupleTableSlot *
ExecInsert(TupleTableSlot *slot,
	TupleTableSlot *planSlot,
	EState *estate,
	bool canSetTag)
{
	HeapTuple	tuple;
	ResultRelInfo *resultRelInfo;
	Relation	resultRelationDesc;
	Oid			newId;
	List	   *recheckIndexes = NIL;

	/*
	 * get the heap tuple out of the tuple table slot, making sure we have a
	 * writable copy
	 */
	tuple = ExecMaterializeSlot(slot);

	/*
	 * get information on the (current) result relation
	 */
	resultRelInfo = estate->es_result_relation_info;
	resultRelationDesc = resultRelInfo->ri_RelationDesc;

	/*
	 * If the result relation has OIDs, force the tuple's OID to zero so that
	 * heap_insert will assign a fresh OID.  Usually the OID already will be
	 * zero at this point, but there are corner cases where the plan tree can
	 * return a tuple extracted literally from some table with the same
	 * rowtype.
	 *
	 * XXX if we ever wanted to allow users to assign their own OIDs to new
	 * rows, this'd be the place to do it.  For the moment, we make a point of
	 * doing this before calling triggers, so that a user-supplied trigger
	 * could hack the OID if desired.
	 */
	if (resultRelationDesc->rd_rel->relhasoids)
		HeapTupleSetOid(tuple, InvalidOid);

	/* BEFORE ROW INSERT Triggers */
	if (resultRelInfo->ri_TrigDesc &&
			resultRelInfo->ri_TrigDesc->trig_insert_before_row)
	{
		slot = ExecBRInsertTriggers(estate, resultRelInfo, slot);

		if (slot == NULL)		/* "do nothing" */
			return NULL;

		/* trigger might have changed tuple */
		tuple = ExecMaterializeSlot(slot);
	}

	/* INSTEAD OF ROW INSERT Triggers */
	if (resultRelInfo->ri_TrigDesc &&
			resultRelInfo->ri_TrigDesc->trig_insert_instead_row)
	{
		slot = ExecIRInsertTriggers(estate, resultRelInfo, slot);

		if (slot == NULL)		/* "do nothing" */
			return NULL;

		/* trigger might have changed tuple */
		tuple = ExecMaterializeSlot(slot);

		newId = InvalidOid;
	}
	else
	{
		/*
		 * Check the constraints of the tuple
		 */
		if (resultRelationDesc->rd_att->constr)
			ExecConstraints(resultRelInfo, slot, estate);

		/*
		 * insert the tuple
		 *
		 * Note: heap_insert returns the tid (location) of the new tuple in
		 * the t_self field.
		 */
		newId = heap_insert(resultRelationDesc, tuple,
				estate->es_output_cid, 0, NULL);

		/*
		 * insert index entries for tuple
		 */
		if (resultRelInfo->ri_NumIndices > 0)
			recheckIndexes = ExecInsertIndexTuples(slot, &(tuple->t_self),
					estate);
	}

	if (canSetTag)
	{
		(estate->es_processed)++;
		estate->es_lastoid = newId;
		setLastTid(&(tuple->t_self));
	}

	/* AFTER ROW INSERT Triggers */
	ExecARInsertTriggers(estate, resultRelInfo, tuple, recheckIndexes);

	list_free(recheckIndexes);

	/* Process RETURNING if present - NO NEED HERE */
	/*
	if (resultRelInfo->ri_projectReturning)
		return ExecProcessReturning(resultRelInfo->ri_projectReturning,
				slot, planSlot);
	*/

	return NULL;
}

/* ----------------------------------------------------------------
 *		InsertIntoTable
 *
 *		Find a table relation given it's name and insert the
 *		tuple on it (updating the indexes and calling the triggers)
 *
 *		Adapted from:
 *		 http://archives.postgresql.org/pgsql-hackers/2008-12/msg01221.php
 *		 and src/backend/commands/copy.c (CopyFrom function)
 * ----------------------------------------------------------------
 */
void InsertIntoTable(const char *table_name, HeapTuple tuple, TupleDesc tupdesc)
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
	/*
	tupdesc = RelationGetDescr(relation);
	*/

	/*
	 * We need a ResultRelInfo so we can use the regular executor's
	 * index-entry-making machinery.
	 */
	resultRelInfo = makeNode(ResultRelInfo);
	resultRelInfo->ri_RangeTableIndex = 1;		/* dummy */
	resultRelInfo->ri_RelationDesc = relation;
	resultRelInfo->ri_TrigDesc = CopyTriggerDesc(relation->trigdesc);
	if (resultRelInfo->ri_TrigDesc)
	{
		resultRelInfo->ri_TrigFunctions = (FmgrInfo *)
			palloc0(resultRelInfo->ri_TrigDesc->numtriggers * sizeof(FmgrInfo));
		resultRelInfo->ri_TrigWhenExprs = (List **)
			palloc0(resultRelInfo->ri_TrigDesc->numtriggers * sizeof(List *));
	}
	resultRelInfo->ri_TrigInstrument = NULL;

	ExecOpenIndices(resultRelInfo);

	estate->es_result_relations = resultRelInfo;
	estate->es_num_result_relations = 1;
	estate->es_result_relation_info = resultRelInfo;

	/* Set up a tuple slot too */
	slot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(slot, tupdesc);
	/* Triggers might need a slot as well */
	estate->es_trig_tuple_slot = ExecInitExtraTupleSlot(estate);

	ExecStoreTuple(tuple, slot, InvalidBuffer, false);
	ExecInsert(slot, slot, estate, false);

	/* Free resources */
	ExecResetTupleTable(estate->es_tupleTable, false);
	ExecCloseIndices(resultRelInfo);
	FreeExecutorState(estate);
	RelationClose(relation);
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
	char *dt;
	int month, year;
	char *relname;
	char child_table_name[NAMEDATALEN];

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

	/* get the date value as a string and retrieve year and month integers */
	dt = SPI_getvalue(rettuple, tupdesc, attrnum);
	sscanf(dt, "%d-%d-%*d", &year, &month);

	/* get the table name, based on the date field */
	if (month >= 1 && month <= 12
		&& snprintf(child_table_name, sizeof(child_table_name), "%s_%04d_%02d", relname, year, month) < 0
	)
		elog(ERROR, "insert_partition: %d-%d invalid year-month", year, month);

	/* insert the tuple on the child table */
	InsertIntoTable(child_table_name, rettuple, tupdesc);

	/* return null, as it has been inserted on a child table */
	return PointerGetDatum(NULL);
}

