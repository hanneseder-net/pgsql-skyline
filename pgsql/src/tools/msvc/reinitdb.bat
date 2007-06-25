set PGROOT=c:\pgsql
set PGDATA=%PGROOT%\db

rmdir /S /Q %PGDATA% 

initdb
