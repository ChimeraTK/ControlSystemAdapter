all: ProcessVariableAdapterRequirements.pdf

.tex.pdf:
	echo -e "\\\newcommand{\svnrevision}{Revision `svnversion -n`}" > svnrevision.tex
	pdflatex $<
	pdflatex $<
	rm svnrevision.tex

.SUFFIXES: .tex .pdf

