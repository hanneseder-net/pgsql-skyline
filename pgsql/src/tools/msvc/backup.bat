set PGROOT=c:\pgsql
set PGDATA=%PGROOT%\db

pg_ctl start -w
pg_dumpall > dumpall.dump
pg_ctl stop -w
