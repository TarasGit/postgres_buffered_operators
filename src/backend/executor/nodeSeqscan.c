/*-------------------------------------------------------------------------
 *
 * nodeSeqscan.c
 *	  Support routines for sequential scans of relations.
 *
 * Portions Copyright (c) 1996-2015, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/executor/nodeSeqscan.c
 *
 *-------------------------------------------------------------------------
 */
/*
 * INTERFACE ROUTINES
 *		ExecSeqScan				sequentially scans a relation.
 *		ExecSeqNext				retrieve next tuple in sequential order.
 *		ExecInitSeqScan			creates and initializes a seqscan node.
 *		ExecEndSeqScan			releases any storage allocated.
 *		ExecReScanSeqScan		rescans the relation
 */
#include "postgres.h"

#include "access/relscan.h"
#include "executor/execdebug.h"
#include "executor/nodeSeqscan.h"
#include "utils/rel.h"

static void InitScanRelation(SeqScanState *node, EState *estate, int eflags);
static void InitScanRelationBuffer(SeqScanState *node, EState *estate, int eflags);
static TupleTableSlot *SeqNext(SeqScanState *node);

/* ----------------------------------------------------------------
 *						Scan Support
 * ----------------------------------------------------------------
 */

/* ----------------------------------------------------------------
 *		SeqNext
 *
 *		This is a workhorse for ExecSeqScan
 * ----------------------------------------------------------------
 */

void
SeqNextListQualTuple(SeqScanState *node)
{
	HeapTuple	tuple;
	HeapScanDesc scandesc;
	EState	   *estate;
	ScanDirection direction;
	//TupleTableSlot *slot;
	TupleTableSlot **slotlist;

	extern int mybuffer_size;
	unsigned int mybuffersize = mybuffer_size;
	unsigned int i;
	int sizectup = sizeof(scandesc->rs_ctup);


	/*
	 * get information from the estate and scan state
	 */
	scandesc = node->ss_currentScanDesc;
	estate = node->ps.state;
	direction = estate->es_direction;
	//slot = node->ss_ScanTupleSlot;
	slotlist = node->ss_ScanTupleSlotList;

	for(i=mybuffersize;i--;){
	/*
	 * get the next tuple from the table
	 */
	tuple = heap_getnext(scandesc, direction);


	memcpy(&scandesc->rs_ctuplist[i],&scandesc->rs_ctup, sizectup);//Taras: what is faster memcpy or memmove ???[MYTODO]

	slotlist[i]->qual = 1;
	/*
	 * save the tuple and the buffer returned to us by the access methods in
	 * our scan tuple slot and return the slot.  Note: we pass 'false' because
	 * tuples returned by heap_getnext() are pointers onto disk pages and were
	 * not created with palloc() and so should not be pfree()'d.  Note also
	 * that ExecStoreTuple will increment the refcount of the buffer; the
	 * refcount will not be dropped until the tuple table slot is cleared.
	 */
		if (tuple){
			ExecStoreTuple(&scandesc->rs_ctuplist[i],	/* tuple to store */
					   slotlist[i],	/* slot to store in */
					   scandesc->rs_cbuf,		/* buffer associated with this
												 * tuple */
					   false);	/* don't pfree this pointer */

		}else{
			ExecClearTuple(slotlist[i]);
			break;
		}
	}
}

static TupleTableSlot * //Taras: original - shall not change
SeqNext(SeqScanState *node)
{
	HeapTuple	tuple;
	HeapScanDesc scandesc;
	EState	   *estate;
	ScanDirection direction;
	TupleTableSlot *slot;

	/*
	 * get information from the estate and scan state
	 */
	scandesc = node->ss_currentScanDesc;
	estate = node->ps.state;
	direction = estate->es_direction;
	slot = node->ss_ScanTupleSlot;

	/*
	 * get the next tuple from the table
	 */
	tuple = heap_getnext(scandesc, direction);

	/*
	 * save the tuple and the buffer returned to us by the access methods in
	 * our scan tuple slot and return the slot.  Note: we pass 'false' because
	 * tuples returned by heap_getnext() are pointers onto disk pages and were
	 * not created with palloc() and so should not be pfree()'d.  Note also
	 * that ExecStoreTuple will increment the refcount of the buffer; the
	 * refcount will not be dropped until the tuple table slot is cleared.
	 */
	if (tuple)
		ExecStoreTuple(tuple,	/* tuple to store */
					   slot,	/* slot to store in */
					   scandesc->rs_cbuf,		/* buffer associated with this
												 * tuple */
					   false);	/* don't pfree this pointer */
	else
		ExecClearTuple(slot);

	return slot;
}

/*
 * SeqRecheck -- access method routine to recheck a tuple in EvalPlanQual
 */
static bool
SeqRecheck(SeqScanState *node, TupleTableSlot *slot)
{
	/*
	 * Note that unlike IndexScan, SeqScan never use keys in heap_beginscan
	 * (and this is very bad) - so, here we do not check are keys ok or not.
	 */
	return true;
}

/* ----------------------------------------------------------------
 *		ExecSeqScan(node)
 *
 *		Scans the relation sequentially and returns the next qualifying
 *		tuple.
 *		We call the ExecScan() routine and pass it the appropriate
 *		access method functions.
 * ----------------------------------------------------------------
 */
TupleTableSlot *
ExecSeqScan(SeqScanState *node)//Taras: original - shall not change
{
	return ExecScan((ScanState *) node,
					(ExecScanAccessMtd) SeqNext,
					(ExecScanRecheckMtd) SeqRecheck);
}

TupleTableSlot **
ExecSeqScanListQualTuple(SeqScanState *node)
{
	return ExecScanListQualTuple((ScanState *) node);//Taras: is not used
}

/* ----------------------------------------------------------------
 *		InitScanRelation
 *
 *		Set up to access the scan relation.
 * ----------------------------------------------------------------
 */
static void //Taras: is not used
InitScanRelation(SeqScanState *node, EState *estate, int eflags)//Taras: original - shall not change
{
	Relation	currentRelation;
	HeapScanDesc currentScanDesc;

	/*
	 * get the relation object id from the relid'th entry in the range table,
	 * open that relation and acquire appropriate lock on it.
	 */
	currentRelation = ExecOpenScanRelation(estate,
									  ((SeqScan *) node->ps.plan)->scanrelid,
										   eflags);

	/* initialize a heapscan */
	currentScanDesc = heap_beginscan(currentRelation,
									 estate->es_snapshot,
									 0,
									 NULL);

	node->ss_currentRelation = currentRelation;
	node->ss_currentScanDesc = currentScanDesc;

	/* and report the scan tuple slot's rowtype */
	ExecAssignScanType(node, RelationGetDescr(currentRelation));
}

static void
InitScanRelationBuffer(SeqScanState *node, EState *estate, int eflags)//Taras: original - shall not change
{
	Relation	currentRelation;
	HeapScanDesc currentScanDesc;

	/*
	 * get the relation object id from the relid'th entry in the range table,
	 * open that relation and acquire appropriate lock on it.
	 */
	currentRelation = ExecOpenScanRelation(estate,
									  ((SeqScan *) node->ps.plan)->scanrelid,
										   eflags);

	/* initialize a heapscan */
	currentScanDesc = heap_beginscanbuffer(currentRelation,//Taras: changed
									 estate->es_snapshot,
									 0,
									 NULL);

	node->ss_currentRelation = currentRelation;
	node->ss_currentScanDesc = currentScanDesc;

	/* and report the scan tuple slot's rowtype */
	ExecAssignScanType(node, RelationGetDescr(currentRelation));
	ExecAssignScanTypeBuffer(node, RelationGetDescr(currentRelation));//Taras: added
}


/* ----------------------------------------------------------------
 *		ExecInitSeqScan
 * ----------------------------------------------------------------
 */
SeqScanState *
ExecInitSeqScan(SeqScan *node, EState *estate, int eflags)
{
	SeqScanState *scanstate;
	extern int mybuffer_size;
	unsigned int mybuffersize = mybuffer_size;
	/*
	 * Once upon a time it was possible to have an outerPlan of a SeqScan, but
	 * not any more.
	 */
	Assert(outerPlan(node) == NULL);
	Assert(innerPlan(node) == NULL);

	/*
	 * create state structure
	 */
	scanstate = makeNode(SeqScanState);
	scanstate->ps.plan = (Plan *) node;
	scanstate->ps.state = estate;

	/*
	 * Miscellaneous initialization
	 *
	 * create expression context for node
	 */
	ExecAssignExprContext(estate, &scanstate->ps);

	/*
	 * initialize child expressions
	 */
	scanstate->ps.targetlist = (List *)
		ExecInitExpr((Expr *) node->plan.targetlist,
					 (PlanState *) scanstate);
	scanstate->ps.qual = (List *)
		ExecInitExpr((Expr *) node->plan.qual,
					 (PlanState *) scanstate);

	/*
	 * tuple table initialization
	 */
	ExecInitResultTupleSlot(estate, &scanstate->ps);
	ExecInitScanTupleSlot(estate, scanstate);

	scanstate->ss_ScanTupleSlotList = MemoryContextAllocZero(CurrentMemoryContext, sizeof(TupleTableSlot*) * mybuffersize);//Taras: added for Scan
	scanstate->ps.ps_ResultTupleSlotList = MemoryContextAllocZero(CurrentMemoryContext, sizeof(TupleTableSlot*) * mybuffersize);//Taras: added for Proj
	scanstate->resultlist =  MemoryContextAllocZero(CurrentMemoryContext, sizeof(TupleTableSlot*) * mybuffersize); //Taras: for output
	scanstate->actualpos = mybuffersize;//Taras: added

	ExecInitResultTupleSlotBuffer(estate, &scanstate->ps);//Taras: added
	ExecInitScanTupleSlotBuffer(estate, scanstate);//Taras: added

	/*
	 * initialize scan relation
	 */
	//InitScanRelation(scanstate, estate, eflags);//Taras:rausgenommer, da gegenseitiger Ausschluss mit -//-Buffer
	InitScanRelationBuffer(scanstate,estate,eflags);//Taras: added

	scanstate->ps.ps_TupFromTlist = false;

	/*
	 * Initialize result tuple type and projection info.
	 */
	ExecAssignResultTypeFromTL(&scanstate->ps);
	//ExecAssignScanProjectionInfo(scanstate);//Taras: rausgenommen, da gegenseitiger Ausschluss mit -//-Buffer

	ExecAssignResultTypeFromTLBuffer(&scanstate->ps);//Taras: added
	ExecAssignScanProjectionInfoBuffer(scanstate);//Taras: added


	return scanstate;
}

/* ----------------------------------------------------------------
 *		ExecEndSeqScan
 *
 *		frees any storage allocated through C routines.
 * ----------------------------------------------------------------
 */
void
ExecEndSeqScan(SeqScanState *node)
{
	Relation	relation;
	HeapScanDesc scanDesc;

	/*
	 * get information from node
	 */
	relation = node->ss_currentRelation;
	scanDesc = node->ss_currentScanDesc;

	/*
	 * Free the exprcontext
	 */
	ExecFreeExprContext(&node->ps);

	/*
	 * clean out the tuple table
	 */
	ExecClearTuple(node->ps.ps_ResultTupleSlot);
	ExecClearTuple(node->ss_ScanTupleSlot);

	/*
	 * close heap scan
	 */
	heap_endscan(scanDesc);

	/*
	 * close the heap relation.
	 */
	ExecCloseScanRelation(relation);
}

/* ----------------------------------------------------------------
 *						Join Support
 * ----------------------------------------------------------------
 */

/* ----------------------------------------------------------------
 *		ExecReScanSeqScan
 *
 *		Rescans the relation.
 * ----------------------------------------------------------------
 */
void
ExecReScanSeqScan(SeqScanState *node)
{
	HeapScanDesc scan;

	scan = node->ss_currentScanDesc;

	heap_rescan(scan,			/* scan desc */
				NULL);			/* new scan keys */

	ExecScanReScan((ScanState *) node);
}
