#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for utils/skyline
#
# IDENTIFICATION
#    $PostgreSQL: $
#
#-------------------------------------------------------------------------

subdir = src/backend/utils/skyline
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

OBJS = tuplewindow.o

all: SUBSYS.o

SUBSYS.o: $(OBJS)
	$(LD) $(LDREL) $(LDOUT) SUBSYS.o $(OBJS)

clean: 
	rm -f SUBSYS.o $(OBJS)
