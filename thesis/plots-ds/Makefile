# -*- Makefile -*- tell emacs it's Makefile
#
.PHONY: all qps clean

GS=gswin32c.exe

dotgraphs=ds-parse.svg

all: qps

qps: $(dotgraphs) 

#$(dotgraphs) : %.pdf : %.ps
#	$(GS) -dBATCH -dNOPAUSE -sDEVICE=pdfwrite -sOutputFile=$@ $<

%.ps: %.dot
	dot -Tps2 $< > $@

%.svg: %.dot
	dotty $< > $@

clean:
	rm -rf *.aux *.log *~ *.bbl *.blg
