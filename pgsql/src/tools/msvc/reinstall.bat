set PGROOT=c:\pgsql
set PGDATA=%PGROOT%\db

pg_ctl start -w
pg_dumpall > dumpall.dump
pg_ctl stop -w

call install.bat %PGROOT%

rmdir /S /Q %PGDATA% 

initdb

pg_ctl start -w
psql -d postgres < dumpall.dump
pg_ctl stop -w
