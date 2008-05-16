# -*- Makefile -*-

.PHONE: all

plots_src = ../../src/profile/log/plots-final
plots := $(wildcard *.pdf)

all : $(plots)

$(plots): %.pdf : $(plots_src)/%.pdf
	cp $< $@
