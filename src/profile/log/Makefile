.PHONY: all clean

QP2LOG=../qp2log
PIVOT_LOG=../pivot_log

csvfiles = sql.csv bnl.sfs.append.csv bnl.sfs.ef.prepend.csv bnl.sfs.prepend.csv presort.csv sort.csv bnl.sfs.ef.append.csv bnl.sfs.ef.ranked.csv bnl.sfs.ranked.csv sfs.index.csv

all: $(csvfiles) master.csv

master.csv: $(csvfiles)
	head -n 1 sql.csv > master.csv
	tail -q -n +2 $(csvfiles) >> master.csv

sql.csv: sql.zip
	unzip -p $< | $(QP2LOG) | $(PIVOT_LOG) --c=method --c=seqscan.o.on=table --c=seqscan.o.rows=outrows --c=skyline.dim=dim --c=seqscan.o.tablesize=inrows --c=skyline.method --c=skyline.passes --c=skyline.windowsize --c=skyline.windowpolicy --c=skyline.cmps.tuples --c=skyline.cmps.fields --c=total > $@

%.csv : %.zip
	unzip -p $< | $(QP2LOG) | $(PIVOT_LOG) --c=method --c=seqscan.on=table --c=skyline.rows=outrows --c=skyline.dim=dim --c=seqscan.rows=inrows --c=skyline.method --c=skyline.passes --c=skyline.windowsize --c=skyline.windowpolicy --c=skyline.cmps.tuples --c=skyline.cmps.fields --c=total > $@
