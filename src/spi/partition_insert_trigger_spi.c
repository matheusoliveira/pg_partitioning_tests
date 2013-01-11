/*-------------------------------------------------------------------------
 *
 * partition_insert_trigger_spi.c
 *	  A function to redirect insertions from parent table to child
 *	  partitions.
 *	  
 * Author: Matheus de Oliveira <matioli (dot) matheus (at) gmail.com>
 *-------------------------------------------------------------------------
 */
#include <postgres.h>
#include <commands/trigger.h>
#include <executor/executor.h>
#include <executor/spi.h>
#include <utils/rel.h>
#include <utils/date.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

typedef struct PreparedPlan {
	char key[NAMEDATALEN];
	SPIPlanPtr plan;
	struct PreparedPlan *next;
} PreparedPlan;

static PreparedPlan * FindPlan(const char *key, PreparedPlan **list) {
	PreparedPlan *item = *list;
	PreparedPlan *last = *list;
	if (*list == NULL) {
		*list = (PreparedPlan *) malloc(sizeof(PreparedPlan));
		strcpy((*list)->key, key);
		(*list)->plan = NULL;
		(*list)->next = NULL;
		return *list;
	}
	while (item != NULL) {
		if (strcmp(item->key, key) == 0) {
			return item;
		}
		last = item;
		item = item->next;
	}
	last->next = item = (PreparedPlan *) malloc(sizeof(PreparedPlan));
	strcpy(item->key, key);
	item->plan = NULL;
	item->next = NULL;
	return item;
}

extern Datum partition_insert_trigger_spi(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(partition_insert_trigger_spi);
/* ----------------------------------------------------------------
 *		partition_insert_trigger_spi
 *
 *		Trigger to redirect the tuple to a child table, based on the field
 *		name passed by the first argument, which should be a date type.
 *		The partition choice is fixed and goes to the table named as:
 *			Format("%s_%04d_%02d", parenttablename, dt year, dt month)
 *
 * ----------------------------------------------------------------
 */
Datum partition_insert_trigger_spi(PG_FUNCTION_ARGS) {
	static PreparedPlan *plans = NULL;
	PreparedPlan *cur_plan;
	SPIPlanPtr tmp_plan;
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	TupleDesc tupdesc;
	HeapTuple rettuple;
	bool isnull;
	int ret, i;
	int attrnum;
	char **args;
	DateADT dt;
	int mday, month, year;
	/* we should use dynamic allocated, but hey, this is just a test */
	char insert_command[2056];
	char child_table_name[NAMEDATALEN];
	char *relname;

	/* make sure it's called as a trigger at all */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "partition_insert: not called by trigger manager");

	if (!TRIGGER_FIRED_BEFORE(trigdata->tg_event))
		elog(ERROR, "partition_insert: should be called as BEFORE");

	tupdesc = trigdata->tg_relation->rd_att;
	rettuple = trigdata->tg_trigtuple;
	relname = RelationGetRelationName(trigdata->tg_relation);
	args = trigdata->tg_trigger->tgargs;

	/* connect to SPI manager */
	if ((ret = SPI_connect()) < 0)
		elog(ERROR, "partition_insert: SPI_connect returned %d", ret);

	/* get the column that contains the partition key */
	attrnum = SPI_fnumber(tupdesc, args[0]);
	if (attrnum <= 0)
		elog(ERROR, "partition_insert: column \"%s\" not found", args[0]);

	/* get the date value and retrieve year, month and day as integers */
	dt = DatumGetDateADT(SPI_getbinval(rettuple, tupdesc, attrnum, &isnull));
	if (DATE_NOT_FINITE(dt))
		elog(ERROR, "insert_partition: date must be finite");
	if (isnull)
		elog(ERROR, "insert_partition: date cannot be NULL");
	j2date(dt + POSTGRES_EPOCH_JDATE, &year, &month, &mday);

	/* generate the beginning of the SQL command to insert into the evaluated table.
	 * We don't care if the child table exists or not here */
	if (month >= 1 && month <= 12
		&& snprintf(child_table_name, sizeof(child_table_name), "%s_%04d_%02d", relname, year, month) < 0
	)
		elog(ERROR, "partition_insert: %d-%d invalid year-month", year, month);
	
	cur_plan = FindPlan(child_table_name, &plans);

	/* declare and allocate the fields arrays */
	Datum *values = SPI_palloc(sizeof(Datum) * tupdesc->natts);
	Oid *types = SPI_palloc(sizeof(Oid) * tupdesc->natts);
	char *nulls = SPI_palloc(sizeof(bool) * tupdesc->natts);
	char tmp[21];
	/* concatenate the fields into the INSERT command and push on the arrays */
	if (!cur_plan->plan) {
		snprintf(insert_command, sizeof(insert_command), "INSERT INTO %s VALUES(", child_table_name);
	}
	for (i = 0; i < tupdesc->natts; i++) {
		values[i] = SPI_getbinval(rettuple, tupdesc, i+1, &isnull);
		nulls[i] = isnull ? 'n': ' ';
		types[i] = SPI_gettypeid(tupdesc, i+1);
		if (!cur_plan->plan) {
			sprintf(tmp, "$%d", i+1);
			if (i)
				strcat(insert_command, ", ");
			strcat(insert_command, tmp);
		}
	}
	if (!cur_plan->plan) {
		strcat(insert_command, ")");
		cur_plan->plan = SPI_prepare(insert_command, tupdesc->natts, types);
		if (cur_plan->plan == NULL)
			elog(ERROR, "partition_insert: SPI_prepare returned %d", SPI_result);

		if (SPI_keepplan(cur_plan->plan))
			elog(ERROR, "partition_insert: SPI_keepplan failed");
	}
	ret = SPI_execute_plan(cur_plan->plan, values, nulls, false, 1);

    if (ret < 0)
        elog(ERROR, "partition_insert: SPI_execute returned %d", ret);

	SPI_finish();

	/* return null, as it has been inserted on a child table */
	return PointerGetDatum(NULL);
}

