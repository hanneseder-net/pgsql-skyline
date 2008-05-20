# -*- Makefile -*-
CURRENTWORKPAGE=25
# set CURRENTWORKPAGE to the page in the .pdf where you are currently
# working on, this is used for pdfopen --page=..

# [NOTE]: all output formats depend on these files
# add new files to this list
FILES=\
	Makefile \
	01-titlepage-en.tex 01-titlepage-de.tex \
	02-acknowledgement.tex \
	03-abstract-en.tex 03-abstract-de.tex \
	04-body.tex \
	heder.sty skyline.bib

TMPFILES=*.aux *.log *.toc *.lot *.lof *.bbl *.blg *.ind *.ilg *.idx *.out *.rai *.rao
TRASHFILES=*~ *.*~

# use in `make realclean` to remove all files
TARGETFILES=00-thesis.pdf

# retrieve subversion revision number of working directory
REVISION := $(shell svn info | perl -n -e 'if ($$_ =~ m/^Revision:\s*(\d+)/) { print "$$1"; }')

# tools options
TEXIFYFLAGS=--src-specials --tex-option="-c-style-errors -halt-on-error"

all: 00-thesis.pdf

# [NOTE] using texify eases the building of tex stuff, it calls
# every thing needed, as many times as needed.


00-thesis.pdf: closepdf prereq 00-thesis.rao 00-thesis.tex $(FILES)
	texify --pdf $(TEXIFYFLAGS) 00-thesis.tex
	pdfopen --file=$@ --page=$(CURRENTWORKPAGE)

closepdf:
	-pdfclose --file=00-thesis.pdf

# [NOTE] on cpp:
# -C			Do not discard comments
# -P			Do not generate #line directives
# -D<macro>[=<val>]     Define a <macro> with <val> as its value. If 
#			just <macro> is given, <val> is taken to be 1

00-thesis.rao: 00-thesis.rai
	rail 00-thesis

00-thesis.rai: prereq 00-thesis.tex $(FILES)
	pdflatex 00-thesis.tex

00-thesis.tex: 00-thesis.skel Makefile
	cpp -C -P -D"OPTIONS=a4paper,10pt,final" $< >$@

clean:
	rm -rf $(TMPFILES) $(TRASHFILES)

distclean: clean
	rm -rf $(TARGETFILES)


prereq: heder.sty rail.sty skyline.bib

heder.sty: ../tex/heder/heder.sty
	cp $< $@

rail.sty: ../tex/rail/rail.sty
	cp $< $@

skyline.bib: ../bibtex/bib/skyline/skyline.bib
	cp $< $@
