This work was done as part of my master thesis on extending
pgsql with the skyline operator [1].

Let me know if you have any questions.

Best,
-Hannes (hannes@hanneseder.net)

[1] see thesis/RTM/thesis-heder-web-200901271.pdf


### content from the old README.TXT, likely obsolete

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
