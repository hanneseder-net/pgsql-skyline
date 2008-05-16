# -*- Makefile -*-

.PHONE: all

plots_src = ../../mathematica/histo/
plots := $(wildcard *.pdf)

all : $(plots)

$(plots): %.pdf : $(plots_src)/%.pdf
	cp $< $@
