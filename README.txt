

pgsql-cvshead:

svn doesn't care about the mtime of a file, it compares the
content. however cvs does care about the mtime of a file. use the
following script to set the mtime (timestamp) accordinglgy:

cd pgsql-cvshead
find -name "CVS" -type d -execdir ~/skyline/bin/resetcvsts.pl  \;

