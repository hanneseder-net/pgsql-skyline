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
TARGETFILES=00-thesis-heder.pdf 00-thesis-jeder.pdf 00-thesis-heder-twoside.pdf 00-thesis-jeder-twoside.pdf


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
	cpp -C -P -D"OPTIONS=a4paper,10pt,final,oneside" -D"AUTHOR=Ing. Bakk.techn. Hannes Eder" $< >$@

00-thesis-jeder.tex: 00-thesis.skel Makefile
	cpp -C -P -D"OPTIONS=a4paper,10pt,final,oneside" -D"AUTHOR=Ing. Bakk.techn. Johann Eder" $< >$@


00-thesis-heder-twoside.tex: 00-thesis.skel Makefile
	cpp -C -P -D"OPTIONS=a4paper,10pt,final,twoside" -D"AUTHOR=Ing. Bakk.techn. Hannes Eder" $< >$@

00-thesis-jeder-twoside.tex: 00-thesis.skel Makefile
	cpp -C -P -D"OPTIONS=a4paper,10pt,final,twoside" -D"AUTHOR=Ing. Bakk.techn. Johann Eder" $< >$@


#
# close pdf
#

close-heder:
	-pdfclose --file=00-thesis-heder.pdf

close-jeder:
	-pdfclose --file=00-thesis-jeder.pdf

close-heder-twoside:
	-pdfclose --file=00-thesis-heder-twoside.pdf

close-jeder-twoside:
	-pdfclose --file=00-thesis-jeder-twoside.pdf


#
# railroad diagrams
#


%.rai: %.tex $(FILES)
	pdflatex $(patsubst %.tex,%,$<)

%.rao: %.rai
	rail $(patsubst %.rai,%,$<)


#
# texify
#

00-thesis-heder.pdf: prereq close-heder 00-thesis-heder.rao 00-thesis-heder.tex $(FILES)
	texify --pdf $(TEXIFYFLAGS) 00-thesis-heder.tex
	pdfopen --file=$@ --page=$(CURRENTWORKPAGE)


00-thesis-jeder.pdf: prereq close-jeder 00-thesis-jeder.rao 00-thesis-jeder.tex $(FILES)
	texify --pdf $(TEXIFYFLAGS) 00-thesis-jeder.tex
	pdfopen --file=$@ --page=$(CURRENTWORKPAGE)

00-thesis-heder-twoside.pdf: prereq close-heder 00-thesis-heder-twoside.rao 00-thesis-heder-twoside.tex $(FILES)
	texify --pdf $(TEXIFYFLAGS) 00-thesis-heder-twoside.tex
	pdfopen --file=$@ --page=$(CURRENTWORKPAGE)


00-thesis-jeder-twoside.pdf: prereq close-jeder 00-thesis-jeder-twoside.rao 00-thesis-jeder-twoside.tex $(FILES)
	texify --pdf $(TEXIFYFLAGS) 00-thesis-jeder-twoside.tex
	pdfopen --file=$@ --page=$(CURRENTWORKPAGE)


#
# clean up
#

clean:
	rm -rf $(TMPFILES) $(TRASHFILES)

distclean: clean
	rm -rf $(TARGETFILES)
