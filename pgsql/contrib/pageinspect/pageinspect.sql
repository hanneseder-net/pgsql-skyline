-- Adjust this setting to control where the objects get created.
SET search_path = public;

--
-- get_raw_page()
--
CREATE OR REPLACE FUNCTION get_raw_page(text, int4)
RETURNS bytea
AS '$libdir/pageinspect', 'get_raw_page'
LANGUAGE C STRICT;

--
-- page_header()
--
CREATE TYPE page_header_type AS (
	lsn text,
	tli smallint,
	flags smallint,
	lower smallint,
	upper smallint,
	special smallint,
	pagesize smallint,
	version smallint
);

CREATE OR REPLACE FUNCTION page_header(bytea)
RETURNS page_header_type
AS '$libdir/pageinspect', 'page_header'
LANGUAGE C STRICT;

--
-- heap_page_items()
--
CREATE TYPE heap_page_items_type AS (
	lp smallint,
	lp_off smallint,
	lp_flags smallint,
	lp_len smallint,
	t_xmin xid,
	t_xmax xid,
	t_field3 int4,
	t_ctid tid,
	t_infomask2 smallint,
	t_infomask smallint,
	t_hoff smallint,
	t_bits text,
	t_oid oid
);

CREATE OR REPLACE FUNCTION heap_page_items(bytea)
RETURNS SETOF heap_page_items_type
AS '$libdir/pageinspect', 'heap_page_items'
LANGUAGE C STRICT;

--
-- bt_metap()
--
CREATE TYPE bt_metap_type AS (
  magic int4,
  version int4,
  root int4,
  level int4,
  fastroot int4,
  fastlevel int4
);

CREATE OR REPLACE FUNCTION bt_metap(text)
RETURNS bt_metap_type
AS '$libdir/pageinspect', 'bt_metap'
LANGUAGE 'C' STRICT;

--
-- bt_page_stats()
--
CREATE TYPE bt_page_stats_type AS (
  blkno int4,
  type char,
  live_items int4,
  dead_items int4,
  avg_item_size float,
  page_size int4,
  free_size int4,
  btpo_prev int4,
  btpo_next int4,
  btpo int4,
  btpo_flags int4
);

CREATE OR REPLACE FUNCTION bt_page_stats(text, int4)
RETURNS bt_page_stats_type
AS '$libdir/pageinspect', 'bt_page_stats'
LANGUAGE 'C' STRICT;

--
-- bt_page_items()
--
CREATE TYPE bt_page_items_type AS (
  itemoffset smallint,
  ctid tid,
  itemlen smallint,
  nulls bool,
  vars bool,
  data text
);

CREATE OR REPLACE FUNCTION bt_page_items(text, int4)
RETURNS SETOF bt_page_items_type
AS '$libdir/pageinspect', 'bt_page_items'
LANGUAGE 'C' STRICT;