/* ----------
 *	pgstat.h
 *
 *	Definitions for the PostgreSQL statistics collector daemon.
 *
 *	Copyright (c) 2001-2007, PostgreSQL Global Development Group
 *
 *	$PostgreSQL: pgsql/src/include/pgstat.h,v 1.61 2007/05/27 17:28:36 tgl Exp $
 * ----------
 */
#ifndef PGSTAT_H
#define PGSTAT_H

#include "libpq/pqcomm.h"
#include "utils/hsearch.h"
#include "utils/rel.h"
#include "utils/timestamp.h"


/* ----------
 * The types of backend -> collector messages
 * ----------
 */
typedef enum StatMsgType
{
	PGSTAT_MTYPE_DUMMY,
	PGSTAT_MTYPE_TABSTAT,
	PGSTAT_MTYPE_TABPURGE,
	PGSTAT_MTYPE_DROPDB,
	PGSTAT_MTYPE_RESETCOUNTER,
	PGSTAT_MTYPE_AUTOVAC_START,
	PGSTAT_MTYPE_VACUUM,
	PGSTAT_MTYPE_ANALYZE,
	PGSTAT_MTYPE_BGWRITER
} StatMsgType;

/* ----------
 * The data type used for counters.
 * ----------
 */
typedef int64 PgStat_Counter;

/* ----------
 * PgStat_TableCounts			The actual per-table counts kept by a backend
 *
 * This struct should contain only actual event counters, because we memcmp
 * it against zeroes to detect whether there are any counts to transmit.
 * It is a component of PgStat_TableStatus (within-backend state) and
 * PgStat_TableEntry (the transmitted message format).
 *
 * Note: for a table, tuples_returned is the number of tuples successfully
 * fetched by heap_getnext, while tuples_fetched is the number of tuples
 * successfully fetched by heap_fetch under the control of bitmap indexscans.
 * For an index, tuples_returned is the number of index entries returned by
 * the index AM, while tuples_fetched is the number of tuples successfully
 * fetched by heap_fetch under the control of simple indexscans for this index.
 *
 * tuples_inserted/tuples_updated/tuples_deleted count attempted actions,
 * regardless of whether the transaction committed.  new_live_tuples and
 * new_dead_tuples are properly adjusted depending on commit or abort.
 * Note that new_live_tuples can be negative!
 * ----------
 */
typedef struct PgStat_TableCounts
{
	PgStat_Counter t_numscans;

	PgStat_Counter t_tuples_returned;
	PgStat_Counter t_tuples_fetched;

	PgStat_Counter t_tuples_inserted;
	PgStat_Counter t_tuples_updated;
	PgStat_Counter t_tuples_deleted;

	PgStat_Counter t_new_live_tuples;
	PgStat_Counter t_new_dead_tuples;

	PgStat_Counter t_blocks_fetched;
	PgStat_Counter t_blocks_hit;
} PgStat_TableCounts;


/* ------------------------------------------------------------
 * Structures kept in backend local memory while accumulating counts
 * ------------------------------------------------------------
 */


/* ----------
 * PgStat_TableStatus			Per-table status within a backend
 *
 * Most of the event counters are nontransactional, ie, we count events
 * in committed and aborted transactions alike.  For these, we just count
 * directly in the PgStat_TableStatus.  However, new_live_tuples and
 * new_dead_tuples must be derived from tuple insertion and deletion counts
 * with awareness of whether the transaction or subtransaction committed or
 * aborted.  Hence, we also keep a stack of per-(sub)transaction status
 * records for every table modified in the current transaction.  At commit
 * or abort, we propagate tuples_inserted and tuples_deleted up to the
 * parent subtransaction level, or out to the parent PgStat_TableStatus,
 * as appropriate.
 * ----------
 */
typedef struct PgStat_TableStatus
{
	Oid			t_id;				/* table's OID */
	bool		t_shared;			/* is it a shared catalog? */
	struct PgStat_TableXactStatus *trans;	/* lowest subxact's counts */
	PgStat_TableCounts t_counts;	/* event counts to be sent */
} PgStat_TableStatus;

/* ----------
 * PgStat_TableXactStatus		Per-table, per-subtransaction status
 * ----------
 */
typedef struct PgStat_TableXactStatus
{
	PgStat_Counter tuples_inserted;	/* tuples inserted in (sub)xact */
	PgStat_Counter tuples_deleted;	/* tuples deleted in (sub)xact */
	int			nest_level;			/* subtransaction nest level */
	/* links to other structs for same relation: */
	struct PgStat_TableXactStatus *upper;	/* next higher subxact if any */
	PgStat_TableStatus *parent;				/* per-table status */
	/* structs of same subxact level are linked here: */
	struct PgStat_TableXactStatus *next;	/* next of same subxact */
} PgStat_TableXactStatus;


/* ------------------------------------------------------------
 * Message formats follow
 * ------------------------------------------------------------
 */


/* ----------
 * PgStat_MsgHdr				The common message header
 * ----------
 */
typedef struct PgStat_MsgHdr
{
	StatMsgType m_type;
	int			m_size;
} PgStat_MsgHdr;

/* ----------
 * Space available in a message.  This will keep the UDP packets below 1K,
 * which should fit unfragmented into the MTU of the lo interface on most
 * platforms. Does anybody care for platforms where it doesn't?
 * ----------
 */
#define PGSTAT_MSG_PAYLOAD	(1000 - sizeof(PgStat_MsgHdr))


/* ----------
 * PgStat_MsgDummy				A dummy message, ignored by the collector
 * ----------
 */
typedef struct PgStat_MsgDummy
{
	PgStat_MsgHdr m_hdr;
} PgStat_MsgDummy;


/* ----------
 * PgStat_TableEntry			Per-table info in a MsgTabstat
 * ----------
 */
typedef struct PgStat_TableEntry
{
	Oid			t_id;
	PgStat_TableCounts t_counts;
} PgStat_TableEntry;

/* ----------
 * PgStat_MsgTabstat			Sent by the backend to report table
 *								and buffer access statistics.
 * ----------
 */
#define PGSTAT_NUM_TABENTRIES  \
	((PGSTAT_MSG_PAYLOAD - sizeof(Oid) - 3 * sizeof(int))  \
	 / sizeof(PgStat_TableEntry))

typedef struct PgStat_MsgTabstat
{
	PgStat_MsgHdr m_hdr;
	Oid			m_databaseid;
	int			m_nentries;
	int			m_xact_commit;
	int			m_xact_rollback;
	PgStat_TableEntry m_entry[PGSTAT_NUM_TABENTRIES];
} PgStat_MsgTabstat;


/* ----------
 * PgStat_MsgTabpurge			Sent by the backend to tell the collector
 *								about dead tables.
 * ----------
 */
#define PGSTAT_NUM_TABPURGE  \
	((PGSTAT_MSG_PAYLOAD - sizeof(Oid) - sizeof(int))  \
	 / sizeof(Oid))

typedef struct PgStat_MsgTabpurge
{
	PgStat_MsgHdr m_hdr;
	Oid			m_databaseid;
	int			m_nentries;
	Oid			m_tableid[PGSTAT_NUM_TABPURGE];
} PgStat_MsgTabpurge;


/* ----------
 * PgStat_MsgDropdb				Sent by the backend to tell the collector
 *								about a dropped database
 * ----------
 */
typedef struct PgStat_MsgDropdb
{
	PgStat_MsgHdr m_hdr;
	Oid			m_databaseid;
} PgStat_MsgDropdb;


/* ----------
 * PgStat_MsgResetcounter		Sent by the backend to tell the collector
 *								to reset counters
 * ----------
 */
typedef struct PgStat_MsgResetcounter
{
	PgStat_MsgHdr m_hdr;
	Oid			m_databaseid;
} PgStat_MsgResetcounter;


/* ----------
 * PgStat_MsgAutovacStart		Sent by the autovacuum daemon to signal
 *								that a database is going to be processed
 * ----------
 */
typedef struct PgStat_MsgAutovacStart
{
	PgStat_MsgHdr m_hdr;
	Oid			m_databaseid;
	TimestampTz m_start_time;
} PgStat_MsgAutovacStart;


/* ----------
 * PgStat_MsgVacuum				Sent by the backend or autovacuum daemon
 *								after VACUUM or VACUUM ANALYZE
 * ----------
 */
typedef struct PgStat_MsgVacuum
{
	PgStat_MsgHdr m_hdr;
	Oid			m_databaseid;
	Oid			m_tableoid;
	bool		m_analyze;
	bool		m_autovacuum;
	TimestampTz m_vacuumtime;
	PgStat_Counter m_tuples;
} PgStat_MsgVacuum;


/* ----------
 * PgStat_MsgAnalyze			Sent by the backend or autovacuum daemon
 *								after ANALYZE
 * ----------
 */
typedef struct PgStat_MsgAnalyze
{
	PgStat_MsgHdr m_hdr;
	Oid			m_databaseid;
	Oid			m_tableoid;
	bool		m_autovacuum;
	TimestampTz m_analyzetime;
	PgStat_Counter m_live_tuples;
	PgStat_Counter m_dead_tuples;
} PgStat_MsgAnalyze;


/* ----------
 * PgStat_MsgBgWriter           Sent by the bgwriter to update statistics.
 * ----------
 */
typedef struct PgStat_MsgBgWriter
{
	PgStat_MsgHdr m_hdr;

	PgStat_Counter  m_timed_checkpoints;
	PgStat_Counter	m_requested_checkpoints;
	PgStat_Counter	m_buf_written_checkpoints;
	PgStat_Counter	m_buf_written_lru;
	PgStat_Counter	m_buf_written_all;
	PgStat_Counter	m_maxwritten_lru;
	PgStat_Counter	m_maxwritten_all;
} PgStat_MsgBgWriter;


/* ----------
 * PgStat_Msg					Union over all possible messages.
 * ----------
 */
typedef union PgStat_Msg
{
	PgStat_MsgHdr msg_hdr;
	PgStat_MsgDummy msg_dummy;
	PgStat_MsgTabstat msg_tabstat;
	PgStat_MsgTabpurge msg_tabpurge;
	PgStat_MsgDropdb msg_dropdb;
	PgStat_MsgResetcounter msg_resetcounter;
	PgStat_MsgAutovacStart msg_autovacuum;
	PgStat_MsgVacuum msg_vacuum;
	PgStat_MsgAnalyze msg_analyze;
	PgStat_MsgBgWriter msg_bgwriter;
} PgStat_Msg;


/* ------------------------------------------------------------
 * Statistic collector data structures follow
 *
 * PGSTAT_FILE_FORMAT_ID should be changed whenever any of these
 * data structures change.
 * ------------------------------------------------------------
 */

#define PGSTAT_FILE_FORMAT_ID	0x01A5BC96

/* ----------
 * PgStat_StatDBEntry			The collector's data per database
 * ----------
 */
typedef struct PgStat_StatDBEntry
{
	Oid			databaseid;
	PgStat_Counter n_xact_commit;
	PgStat_Counter n_xact_rollback;
	PgStat_Counter n_blocks_fetched;
	PgStat_Counter n_blocks_hit;
	PgStat_Counter n_tuples_returned;
	PgStat_Counter n_tuples_fetched;
	PgStat_Counter n_tuples_inserted;
	PgStat_Counter n_tuples_updated;
	PgStat_Counter n_tuples_deleted;
	TimestampTz last_autovac_time;

	/*
	 * tables must be last in the struct, because we don't write the pointer
	 * out to the stats file.
	 */
	HTAB	   *tables;
} PgStat_StatDBEntry;


/* ----------
 * PgStat_StatTabEntry			The collector's data per table (or index)
 * ----------
 */
typedef struct PgStat_StatTabEntry
{
	Oid			tableid;

	PgStat_Counter numscans;

	PgStat_Counter tuples_returned;
	PgStat_Counter tuples_fetched;

	PgStat_Counter tuples_inserted;
	PgStat_Counter tuples_updated;
	PgStat_Counter tuples_deleted;

	PgStat_Counter n_live_tuples;
	PgStat_Counter n_dead_tuples;
	PgStat_Counter last_anl_tuples;

	PgStat_Counter blocks_fetched;
	PgStat_Counter blocks_hit;

	TimestampTz vacuum_timestamp;		/* user initiated vacuum */
	TimestampTz autovac_vacuum_timestamp;		/* autovacuum initiated */
	TimestampTz analyze_timestamp;		/* user initiated */
	TimestampTz autovac_analyze_timestamp;		/* autovacuum initiated */
} PgStat_StatTabEntry;


/*
 * Global statistics kept in the stats collector
 */
typedef struct PgStat_GlobalStats
{
	PgStat_Counter  timed_checkpoints;
	PgStat_Counter  requested_checkpoints;
	PgStat_Counter  buf_written_checkpoints;
	PgStat_Counter  buf_written_lru;
	PgStat_Counter  buf_written_all;
	PgStat_Counter  maxwritten_lru;
	PgStat_Counter  maxwritten_all;
} PgStat_GlobalStats;


/* ----------
 * Shared-memory data structures
 * ----------
 */

/* Max length of st_activity string ... perhaps replace with a GUC var? */
#define PGBE_ACTIVITY_SIZE	1024

/* ----------
 * PgBackendStatus
 *
 * Each live backend maintains a PgBackendStatus struct in shared memory
 * showing its current activity.  (The structs are allocated according to
 * BackendId, but that is not critical.)  Note that the collector process
 * has no involvement in, or even access to, these structs.
 * ----------
 */
typedef struct PgBackendStatus
{
	/*
	 * To avoid locking overhead, we use the following protocol: a backend
	 * increments st_changecount before modifying its entry, and again after
	 * finishing a modification.  A would-be reader should note the value of
	 * st_changecount, copy the entry into private memory, then check
	 * st_changecount again.  If the value hasn't changed, and if it's even,
	 * the copy is valid; otherwise start over.  This makes updates cheap
	 * while reads are potentially expensive, but that's the tradeoff we want.
	 */
	int			st_changecount;

	/* The entry is valid iff st_procpid > 0, unused if st_procpid == 0 */
	int			st_procpid;

	/* Times when current backend, transaction, and activity started */
	TimestampTz st_proc_start_timestamp;
	TimestampTz st_txn_start_timestamp;
	TimestampTz st_activity_start_timestamp;

	/* Database OID, owning user's OID, connection client address */
	Oid			st_databaseid;
	Oid			st_userid;
	SockAddr	st_clientaddr;

	/* Is backend currently waiting on an lmgr lock? */
	bool		st_waiting;

	/* current command string; MUST be null-terminated */
	char		st_activity[PGBE_ACTIVITY_SIZE];
} PgBackendStatus;


/* ----------
 * GUC parameters
 * ----------
 */
extern bool pgstat_collect_startcollector;
extern bool pgstat_collect_resetonpmstart;
extern bool pgstat_collect_tuplelevel;
extern bool pgstat_collect_blocklevel;
extern bool pgstat_collect_querystring;

/*
 * BgWriter statistics counters are updated directly by bgwriter and bufmgr
 */
extern PgStat_MsgBgWriter BgWriterStats;

/* ----------
 * Functions called from postmaster
 * ----------
 */
extern Size BackendStatusShmemSize(void);
extern void CreateSharedBackendStatus(void);

extern void pgstat_init(void);
extern int	pgstat_start(void);
extern void pgstat_reset_all(void);
extern void allow_immediate_pgstat_restart(void);
#ifdef EXEC_BACKEND
extern void PgstatCollectorMain(int argc, char *argv[]);
#endif


/* ----------
 * Functions called from backends
 * ----------
 */
extern void pgstat_ping(void);

extern void pgstat_report_tabstat(bool force);
extern void pgstat_vacuum_tabstat(void);
extern void pgstat_drop_database(Oid databaseid);
extern void pgstat_drop_relation(Oid relid);

extern void pgstat_clear_snapshot(void);
extern void pgstat_reset_counters(void);

extern void pgstat_report_autovac(Oid dboid);
extern void pgstat_report_vacuum(Oid tableoid, bool shared,
					 bool analyze, PgStat_Counter tuples);
extern void pgstat_report_analyze(Oid tableoid, bool shared,
					  PgStat_Counter livetuples,
					  PgStat_Counter deadtuples);

extern void pgstat_initialize(void);
extern void pgstat_bestart(void);

extern void pgstat_report_activity(const char *what);
extern void pgstat_report_txn_timestamp(TimestampTz tstamp);
extern void pgstat_report_waiting(bool waiting);

extern void pgstat_initstats(Relation rel);

/* nontransactional event counts are simple enough to inline */

#define pgstat_count_heap_scan(rel)										\
	do {																\
		if (pgstat_collect_tuplelevel && (rel)->pgstat_info != NULL)	\
			(rel)->pgstat_info->t_counts.t_numscans++;					\
	} while (0)
/* kluge for bitmap scans: */
#define pgstat_discount_heap_scan(rel)									\
	do {																\
		if (pgstat_collect_tuplelevel && (rel)->pgstat_info != NULL)	\
			(rel)->pgstat_info->t_counts.t_numscans--;					\
	} while (0)
#define pgstat_count_heap_getnext(rel)									\
	do {																\
		if (pgstat_collect_tuplelevel && (rel)->pgstat_info != NULL)	\
			(rel)->pgstat_info->t_counts.t_tuples_returned++;			\
	} while (0)
#define pgstat_count_heap_fetch(rel)									\
	do {																\
		if (pgstat_collect_tuplelevel && (rel)->pgstat_info != NULL)	\
			(rel)->pgstat_info->t_counts.t_tuples_fetched++;			\
	} while (0)
#define pgstat_count_index_scan(rel)									\
	do {																\
		if (pgstat_collect_tuplelevel && (rel)->pgstat_info != NULL)	\
			(rel)->pgstat_info->t_counts.t_numscans++;					\
	} while (0)
#define pgstat_count_index_tuples(rel, n)								\
	do {																\
		if (pgstat_collect_tuplelevel && (rel)->pgstat_info != NULL)	\
			(rel)->pgstat_info->t_counts.t_tuples_returned += (n);		\
	} while (0)
#define pgstat_count_buffer_read(rel)									\
	do {																\
		if (pgstat_collect_blocklevel && (rel)->pgstat_info != NULL)	\
			(rel)->pgstat_info->t_counts.t_blocks_fetched++;			\
	} while (0)
#define pgstat_count_buffer_hit(rel)									\
	do {																\
		if (pgstat_collect_blocklevel && (rel)->pgstat_info != NULL)	\
			(rel)->pgstat_info->t_counts.t_blocks_hit++;				\
	} while (0)

extern void pgstat_count_heap_insert(Relation rel);
extern void pgstat_count_heap_update(Relation rel);
extern void pgstat_count_heap_delete(Relation rel);

extern void AtEOXact_PgStat(bool isCommit);
extern void AtEOSubXact_PgStat(bool isCommit, int nestDepth);

extern void AtPrepare_PgStat(void);
extern void PostPrepare_PgStat(void);

extern void pgstat_twophase_postcommit(TransactionId xid, uint16 info,
									   void *recdata, uint32 len);
extern void pgstat_twophase_postabort(TransactionId xid, uint16 info,
									  void *recdata, uint32 len);

extern void pgstat_send_bgwriter(void);

/* ----------
 * Support functions for the SQL-callable functions to
 * generate the pgstat* views.
 * ----------
 */
extern PgStat_StatDBEntry *pgstat_fetch_stat_dbentry(Oid dbid);
extern PgStat_StatTabEntry *pgstat_fetch_stat_tabentry(Oid relid);
extern PgBackendStatus *pgstat_fetch_stat_beentry(int beid);
extern int	pgstat_fetch_stat_numbackends(void);
extern PgStat_GlobalStats *pgstat_fetch_global(void);

#endif   /* PGSTAT_H */