.PHONY: all clean

QP2LOG=../qp2log
PIVOT_LOG=../pivot_log

masterfiles = presort.csv sort.csv sql.csv bnl.sfs.append.csv bnl.sfs.prepend.csv bnl.sfs.ranked.csv bnl.sfs.ef.append.csv bnl.sfs.ef.prepend.csv bnl.sfs.ef.ranked.csv sfs.index.csv

all: $(masterfiles) master.csv ef.csv wnd.csv

ef.csv: bnl.sfs.ef.append.ranked.ef.zip
	unzip -p $< | $(QP2LOG) | $(PIVOT_LOG) --c=method --c=seqscan.on=table --c=skyline.rows=outrows --c=skyline.dim=dim --c=seqscan.rows=inrows --c=skyline.method --c=skyline.passes --c=skyline.windowsize --c=skyline.windowpolicy --c=skyline.cmps.tuples --c=skyline.cmps.fields --c=elimfilter.windowslots --c=elimfilter.windowpolicy --c=total > $@

wnd.csv: bnl.sfs.append.wnd.zip
	unzip -p $< | $(QP2LOG) | $(PIVOT_LOG) --c=method --c=seqscan.on=table --c=skyline.rows=outrows --c=skyline.dim=dim --c=seqscan.rows=inrows --c=skyline.method --c=skyline.passes --c=skyline.windowsize --c=skyline.windowpolicy --c=skyline.cmps.tuples --c=skyline.cmps.fields --c=skyline.windowslots --c=total > $@

master.csv: $(masterfiles)
	head -n 1 sql.csv > master.csv
	tail -q -n +2 $(masterfiles) >> master.csv

sql.csv: sql.zip
	unzip -p $< | $(QP2LOG) | $(PIVOT_LOG) --c=method --c=seqscan.o.on=table --c=seqscan.o.rows=outrows --c=skyline.dim=dim --c=seqscan.o.tablesize=inrows --c=skyline.method --c=skyline.passes --c=skyline.windowsize --c=skyline.windowpolicy --c=skyline.cmps.tuples --c=skyline.cmps.fields --c=total > $@

%.csv : %.zip
	unzip -p $< | $(QP2LOG) | $(PIVOT_LOG) --c=method --c=seqscan.on=table --c=skyline.rows=outrows --c=skyline.dim=dim --c=seqscan.rows=inrows --c=skyline.method --c=skyline.passes --c=skyline.windowsize --c=skyline.windowpolicy --c=skyline.cmps.tuples --c=skyline.cmps.fields --c=total > $@
