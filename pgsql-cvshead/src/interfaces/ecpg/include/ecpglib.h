/*
 * this is a small part of c.h since we don't want to leak all postgres
 * definitions into ecpg programs
 * $PostgreSQL: pgsql/src/interfaces/ecpg/include/ecpglib.h,v 1.71 2007/08/14 10:01:52 meskes Exp $
 */

#ifndef _ECPGLIB_H
#define _ECPGLIB_H

#include "libpq-fe.h"
#include "ecpgtype.h"
#include "sqlca.h"
#include <string.h>

#ifndef __cplusplus
#ifndef bool
#define bool char
#endif   /* ndef bool */

#ifndef true
#define true	((bool) 1)
#endif   /* ndef true */
#ifndef false
#define false	((bool) 0)
#endif   /* ndef false */
#endif   /* not C++ */

#ifndef TRUE
#define TRUE	1
#endif   /* TRUE */

#ifndef FALSE
#define FALSE	0
#endif   /* FALSE */

#ifdef __cplusplus
extern		"C"
{
#endif

void		ECPGinit_sqlca(struct sqlca_t * sqlca);
void		ECPGdebug(int, FILE *);
bool		ECPGstatus(int, const char *);
bool		ECPGsetcommit(int, const char *, const char *);
bool		ECPGsetconn(int, const char *);
bool		ECPGconnect(int, int, const char *, const char *, const char *, const char *, int);
bool		ECPGdo(const int, const int, const int, const char *, const char, const enum ECPG_statement_type, const char *,...);
bool		ECPGtrans(int, const char *, const char *);
bool		ECPGdisconnect(int, const char *);
bool		ECPGprepare(int, const char *, const int, const char *, const char *);
bool		ECPGauto_prepare(int, const char *, const int, char **, const char *);
bool		ECPGdeallocate(int, int, const char *);
bool		ECPGdeallocate_all(int, int);
char	   *ECPGprepared_statement(const char *, int);

void		ECPGlog(const char *format,...);
char	   *ECPGerrmsg(void);

 /* print an error message */
void		sqlprint(void);

/* define this for simplicity as well as compatibility */

#define		  SQLCODE	 sqlca.sqlcode

/* dynamic SQL */

bool ECPGdo_descriptor(int line, const char *connection,
				  const char *descriptor, const char *query);
bool		ECPGdeallocate_desc(int line, const char *name);
bool		ECPGallocate_desc(int line, const char *name);
bool		ECPGget_desc_header(int, const char *, int *);
bool		ECPGget_desc(int, const char *, int,...);
bool		ECPGset_desc_header(int, const char *, int);
bool		ECPGset_desc(int, const char *, int,...);

void		ECPGset_noind_null(enum ECPGttype, void *);
bool		ECPGis_noind_null(enum ECPGttype, void *);
bool		ECPGdescribe(int, bool, const char *,...);

/* dynamic result allocation */
void		ECPGfree_auto_mem(void);

#ifdef ENABLE_THREAD_SAFETY
void		ecpg_pthreads_init();
#endif

#ifdef __cplusplus
}
#endif

#endif   /* _ECPGLIB_H */
