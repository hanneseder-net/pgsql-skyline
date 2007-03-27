FILES=\
	Makefile \
	00-thesis.tex \
	01-cover-en.tex 01-cover-de.tex \
	02-acknowledgement.tex \
	03-abstract-en.tex 03-abstract-de.tex \
	04-introduction.tex \
	08-summary.tex \
	heder.sty ../bib/skyline.bib

TMPFILES=*.aux *.log *.toc *.lot *.lof *.bbl *.blg *.ind *.ilg *.idx *.out 
BUILDDIR=.
OUTPUTDIR=./output
REVISION := $(shell svn info | perl -n -e 'if ($$_ =~ m/^Revision:\s*(\d+)/) { print "$$1"; }')


draft: $(BUILDDIR)/00-thesis-draft.dvi

final: $(BUILDDIR)/00-thesis-final.dvi

dvi: draft final

putonline: $(OUTPUTDIR)/thesis-$(REVISION).pdf $(OUTPUTDIR)/thesis-draft-$(REVISION).pdf

$(OUTPUTDIR)/thesis-$(REVISION).pdf: $(BUILDDIR)/00-thesis-final.pdf
	cp $< $@

$(OUTPUTDIR)/thesis-draft-$(REVISION).pdf: $(BUILDDIR)/00-thesis-draft.pdf
	cp $< $@

$(BUILDDIR)/00-thesis-final.pdf: $(FILES)
	texify --pdf --tex-option="-output-directory=$(BUILDDIR)" 00-thesis-final.tex

$(BUILDDIR)/00-thesis-final.dvi: $(FILES)
	texify  --tex-option="-output-directory=$(BUILDDIR)" 00-thesis-final.tex

$(BUILDDIR)/00-thesis-draft.pdf: $(FILES)
	texify --pdf --tex-option="-output-directory=$(BUILDDIR)" 00-thesis-draft.tex

$(BUILDDIR)/00-thesis-draft.dvi: $(FILES)
	texify  --tex-option="-output-directory=$(BUILDDIR)" 00-thesis-draft.tex

clean:
	pushd $(BUILDDIR) && rm -rf $(TMPFILES) && popd
