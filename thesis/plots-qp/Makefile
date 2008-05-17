# -*- Makefile -*- tell emacs it's Makefile
#
.PHONY: all qps clean

GS=gswin32c.exe

dotgraphs=qp-sfs-ef.pdf qp-sfs-index.pdf qp-sfs-index-ef.pdf qp-bnl-ef.pdf qp-agg-bnl-sort.pdf

all: qps

qps: $(dotgraphs)

$(dotgraphs) : %.pdf : %.ps
	$(GS) -dBATCH -dNOPAUSE -sDEVICE=pdfwrite -sOutputFile=$@ $<

%.ps: %.dot
	dot -Tps2 $< > $@

clean:
	rm -rf *.aux *.log *~ *.bbl *.blg
