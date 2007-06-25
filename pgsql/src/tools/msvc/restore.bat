set PGROOT=c:\pgsql
set PGDATA=%PGROOT%\db

pg_ctl start -w
psql -d postgres < dumpall.dump
pg_ctl stop -w
