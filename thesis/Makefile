# -*- Makefile -*-
CURRENTWORKPAGE=1
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

TMPFILES=*.aux *.log *.toc *.lot *.lof *.lol *.bbl *.blg *.ind *.ilg *.idx *.out *.rai *.rao
TRASHFILES=*~ *.*~

# use in `make distclean` to remove all files
TARGETFILES=00-thesis-heder.pdf 00-thesis-jeder.pdf

# retrieve subversion revision number of working directory
REVISION := $(shell svn info | perl -n -e 'if ($$_ =~ m/^Revision:\s*(\d+)/) { print "$$1"; }')

# tools options
TEXIFYFLAGS=--src-specials --tex-option="-c-style-errors -halt-on-error"

all: $(TARGETFILES)

# [NOTE] using texify eases the building of tex stuff, it calls
# every thing needed, as many times as needed.


#
# Prerequirements
# 

prereq: heder.sty rail.sty skyline.bib

heder.sty: ../tex/heder/heder.sty
	cp $< $@

rail.sty: ../tex/rail/rail.sty
	cp $< $@

skyline.bib: ../bibtex/bib/skyline/skyline.bib
	cp $< $@


#
# Skeleton to TeX
#

# [NOTE] on cpp:
# -C			Do not discard comments
# -P			Do not generate #line directives
# -D<macro>[=<val>]     Define a <macro> with <val> as its value. If 
#			just <macro> is given, <val> is taken to be 1

00-thesis-heder.tex: 00-thesis.skel Makefile
	cpp -C -P -D"OPTIONS=a4paper,10pt,final" $< >$@

00-thesis-jeder.tex: 00-thesis.skel Makefile
	cpp -C -P -D"OPTIONS=a4paper,10pt,final" -D"AUTHOR_HEDER=0" $< >$@


#
# close pdf
#

close-heder:
	-pdfclose --file=00-thesis-heder.pdf

close-jeder:
	-pdfclose --file=00-thesis-jeder.pdf


#
# railroad diagrams
#

00-thesis-heder.rai: prereq 00-thesis-heder.tex $(FILES)
	pdflatex 00-thesis-heder

00-thesis-heder.rao: 00-thesis-heder.rai
	rail 00-thesis-heder

00-thesis-jeder.rai: prereq 00-thesis-jeder.tex $(FILES)
	pdflatex 00-thesis-jeder

00-thesis-jeder.rao: 00-thesis-jeder.rai
	rail 00-thesis-jeder


#
# texify
#

00-thesis-heder.pdf: prereq close-heder 00-thesis-heder.rao 00-thesis-heder.tex $(FILES)
	texify --pdf $(TEXIFYFLAGS) 00-thesis-heder.tex
	pdfopen --file=$@ --page=$(CURRENTWORKPAGE)


00-thesis-jeder.pdf: prereq close-jeder 00-thesis-jeder.rao 00-thesis-jeder.tex $(FILES)
	texify --pdf $(TEXIFYFLAGS) 00-thesis-jeder.tex
	pdfopen --file=$@ --page=$(CURRENTWORKPAGE)


#
# clean up
#

clean:
	rm -rf $(TMPFILES) $(TRASHFILES)

distclean: clean
	rm -rf $(TARGETFILES)
