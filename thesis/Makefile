# -*- Makefile -*-

# [NOTE]: all output formats depend on these files
# add new files to this list
FILES=\
	Makefile \
	00-thesis.tex \
	01-titlepage-en.tex 01-titlepage-de.tex \
	02-acknowledgement.tex \
	03-abstract-en.tex 03-abstract-de.tex \
	07-results.tex \
	07-01-railroad-diagrams.tex \
	04-introduction.tex \
	08-summary.tex

TMPFILES=*.aux *.log *.toc *.lot *.lof *.bbl *.blg *.ind *.ilg *.idx *.out
TRASHFILES=*~ *.*~

# use in `make realclean` to remove all files
TARGETFILES=00-thesis-draft.dvi 00-thesis-final.dvi 00-thesis-draft.pdf 00-thesis-final.pdf

## [NOTE] using:
#
# BUILDDIR=. 
#
## in conjunction with 
#
# texify --tex-option="-output-directory=$(BUILDDIR)"
#
## has no benefit, since change BUILDDIR to something 
## differnt to . is not recogniced by texify so it
## looks in the wrong directory for the files

# [TODO]: copy output to the web-server
UPLOADDIR=./output

# retrieve subversion revision number of working directory
REVISION := $(shell svn info | perl -n -e 'if ($$_ =~ m/^Revision:\s*(\d+)/) { print "$$1"; }')

# tools options
TEXIFYFLAGS=--tex-option="-c-style-errors -halt-on-error"

# [NOTE] this is first, because we want `make` just build the draft
draft: 00-thesis-draft.dvi
	echo $(BIBINPUTS)

all: dvi pdf

dvi: draft final

final: 00-thesis-final.dvi

pdf: draft-pdf final-pdf

draft-pdf: 00-thesis-draft.pdf

final-pdf: 00-thesis-final.pdf

# use this target pre-commit, to verify the source
check: check-preclean check-build check-postclean

check-preclean:
	rm -rf $(TMPFILES) $(TRASHFILES) $(TARGETFILES)

check-build: all

check-postclean:
	rm -rf $(TMPFILES) $(TRASHFILES) $(TARGETFILES)

upload: $(UPLOADDIR)/thesis-$(REVISION).pdf $(UPLOADDIR)/thesis-draft-$(REVISION).pdf

$(UPLOADDIR)/thesis-$(REVISION).pdf: 00-thesis-final.pdf
	cp $< $@

$(UPLOADDIR)/thesis-draft-$(REVISION).pdf: 00-thesis-draft.pdf
	cp $< $@

# [NOTE] using texify eases the building of tex stuff, it calls
# every thing needed, as many times as needed.

00-thesis-draft.dvi: 00-thesis-draft.tex $(FILES) 
	texify  --src --run-viewer $(TEXIFYFLAGS) $<

00-thesis-final.dvi: 00-thesis-final.tex $(FILES)
	texify  --src $(TEXIFYFLAGS) $<

00-thesis-draft.pdf: 00-thesis-draft.tex $(FILES) 
	texify --pdf $(TEXIFYFLAGS) $<

00-thesis-final.pdf: 00-thesis-final.tex $(FILES)
	texify --pdf $(TEXIFYFLAGS) $<

# [NOTE] on cpp:
# -C			Do not discard comments
# -P			Do not generate #line directives
# -D<macro>[=<val>]     Define a <macro> with <val> as its value. If 
#			just <macro> is given, <val> is taken to be 1

00-thesis-draft.tex: 00-thesis.skel Makefile
	cpp -C -P -D"FILE=00-thesis" -D"OPTIONS=a4paper,10pt,revisions" $< >$@

00-thesis-final.tex: 00-thesis.skel Makefile
	cpp -C -P -D"FILE=00-thesis" -D"OPTIONS=a4paper,10pt,final" $< >$@

clean:
	rm -rf $(TMPFILES) $(TRASHFILES)

distclean: clean
	rm -rf $(TARGETFILES)
