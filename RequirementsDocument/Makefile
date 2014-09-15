SHELL=/bin/bash

all: ControlSystemAdapterRequirements.pdf

.tex.pdf:
	${shell echo -e "\\\newcommand{\svnrevision}{Revision `svnversion -n`}" > svnrevision.tex}
	${shell echo -e "\\\newcommand{\svnshortrev}{r`svnversion -n`}" >> svnrevision.tex}
	pdflatex $<
	pdflatex $<
	rm svnrevision.tex

.SUFFIXES: .tex .pdf

clean:
	rm -rf *.aux *.log *.out *.pdf
