SHELL=/bin/bash

all: ProcessVariableAdapterRequirements.pdf

.tex.pdf:
	${shell echo -e "\\\newcommand{\svnrevision}{Revision `svnversion -n`}" > svnrevision.tex}
	pdflatex $<
	pdflatex $<
	rm svnrevision.tex

.SUFFIXES: .tex .pdf

