set PGROOT=c:\pgsql
set PGDATA=%PGROOT%\db

rem remove tables which are recreated 
rem befor running backup, use:
rem select 'drop table ' || relname || ';' from pg_class where relname ~ '[ace][0-9]d[0-9]+.*';

pg_ctl start -w
pg_dumpall > dumpall.dump
pg_ctl stop -w
