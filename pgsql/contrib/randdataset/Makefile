# $PostgreSQL: $

MODULE_big = randdataset
PG_CPPFLAGS = -I$(libpq_srcdir)
DATA_built = randdataset.sql
DATA = uninstall_randdataset.sql
DOCS = README.randdataset
OBJS = randdataset.o ctxrand.o

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/randdataset
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
