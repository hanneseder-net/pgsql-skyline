/*-------------------------------------------------------------------------
 *
 * fe-auth.h
 *
 *	  Definitions for network authentication routines
 *
 * Portions Copyright (c) 1996-2007, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/interfaces/libpq/fe-auth.h,v 1.26 2007/01/05 22:20:00 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef FE_AUTH_H
#define FE_AUTH_H

#include "libpq-fe.h"
#include "libpq-int.h"


extern int pg_fe_sendauth(AuthRequest areq, PGconn *conn, const char *hostname,
			   const char *password, char *PQerrormsg);
extern char *pg_fe_getauthname(char *PQerrormsg);

#endif   /* FE_AUTH_H */
