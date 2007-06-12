

pgsql-cvshead:

svn doesn't care about the mtime of a file, it compares the
content. however cvs does care about the mtime of a file. use the
following script to set the mtime (timestamp) accordinglgy:

cd pgsql-cvshead
find -name "CVS" -type d -execdir ~/skyline/bin/resetcvsts.pl  \;

to minimize changes to the CVS\Entries* files run
~/skyline/bin/cvs-entries-normalize.sh. The Entries Files are sorted
and there be reflecing just real changes and the diff is more readable
since updates appeare as updates and not as delete/inserts

