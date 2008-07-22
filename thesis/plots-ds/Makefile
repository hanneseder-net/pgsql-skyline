# -*- Makefile -*- tell emacs it's Makefile
#
.PHONY: all qps clean

GS=gswin32c.exe

dotgraphs=ds-parse.pdf

all: qps

qps: $(dotgraphs)

$(dotgraphs) : %.pdf : %.ps
	$(GS) -dBATCH -dNOPAUSE -sDEVICE=pdfwrite -sOutputFile=$@ $<

%.ps: %.dot
	neato -Tps2 $< > $@

clean:
	rm -rf *.aux *.log *~ *.bbl *.blg
