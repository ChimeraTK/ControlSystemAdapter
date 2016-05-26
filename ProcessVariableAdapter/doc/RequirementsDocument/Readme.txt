You need pdflatex for this document.

The document uses a Makefile to automatically include the svn revision number. It requires the svn command to be in the search path, as well as make and pdflatex.

Workaround if the Makefile does not work (Windows?):
Create a file named 'svnrevision.tex' with the content

\snvrevision{Revision: Not supported}

and put into this folder. Afterwards pdflatex should work, also from any GUI. You have no versioning, though.
