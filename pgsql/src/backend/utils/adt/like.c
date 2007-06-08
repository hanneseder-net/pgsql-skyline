/*-------------------------------------------------------------------------
 *
 * like.c
 *	  like expression handling code.
 *
 *	 NOTES
 *		A big hack of the regexp.c code!! Contributed by
 *		Keith Parks <emkxp01@mtcc.demon.co.uk> (7/95).
 *
 * Portions Copyright (c) 1996-2007, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	$PostgreSQL: pgsql/src/backend/utils/adt/like.c,v 1.69 2007/06/02 02:03:42 adunstan Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>

#include "mb/pg_wchar.h"
#include "utils/builtins.h"


#define LIKE_TRUE						1
#define LIKE_FALSE						0
#define LIKE_ABORT						(-1)


static int	SB_MatchText(char *t, int tlen, char *p, int plen);
static text *SB_do_like_escape(text *, text *);

static int	MB_MatchText(char *t, int tlen, char *p, int plen);
static text *MB_do_like_escape(text *, text *);

static int	UTF8_MatchText(char *t, int tlen, char *p, int plen);

static int	GenericMatchText(char *s, int slen, char* p, int plen);
static int	Generic_Text_IC_like(text *str, text *pat);

/*--------------------
 * Support routine for MatchText. Compares given multibyte streams
 * as wide characters. If they match, returns 1 otherwise returns 0.
 *--------------------
 */
static inline int
wchareq(char *p1, char *p2)
{
	int			p1_len;

	/* Optimization:  quickly compare the first byte. */
	if (*p1 != *p2)
		return 0;

	p1_len = pg_mblen(p1);
	if (pg_mblen(p2) != p1_len)
		return 0;

	/* They are the same length */
	while (p1_len--)
	{
		if (*p1++ != *p2++)
			return 0;
	}
	return 1;
}

/*
 * Formerly we had a routine iwchareq() here that tried to do case-insensitive
 * comparison of multibyte characters.	It did not work at all, however,
 * because it relied on tolower() which has a single-byte API ... and
 * towlower() wouldn't be much better since we have no suitably cheap way
 * of getting a single character transformed to the system's wchar_t format.
 * So now, we just downcase the strings using lower() and apply regular LIKE
 * comparison.	This should be revisited when we install better locale support.
 */

#define NextByte(p, plen)	((p)++, (plen)--)

/* Set up to compile like_match.c for multibyte characters */
#define CHAREQ(p1, p2) wchareq((p1), (p2))
#define NextChar(p, plen) \
	do { int __l = pg_mblen(p); (p) +=__l; (plen) -=__l; } while (0)
#define CopyAdvChar(dst, src, srclen) \
	do { int __l = pg_mblen(src); \
		 (srclen) -= __l; \
		 while (__l-- > 0) \
			 *(dst)++ = *(src)++; \
	   } while (0)

#define MatchText	MB_MatchText
#define do_like_escape	MB_do_like_escape

#include "like_match.c"

/* Set up to compile like_match.c for single-byte characters */
#define CHAREQ(p1, p2) (*(p1) == *(p2))
#define NextChar(p, plen) NextByte((p), (plen))
#define CopyAdvChar(dst, src, srclen) (*(dst)++ = *(src)++, (srclen)--)

#define MatchText	SB_MatchText
#define do_like_escape	SB_do_like_escape

#include "like_match.c"


/* setup to compile like_match.c for UTF8 encoding, using fast NextChar */

#define NextChar(p, plen) \
	do { (p)++; (plen)--; } while ((plen) > 0 && (*(p) & 0xC0) == 0x80 ) 
#define MatchText	UTF8_MatchText

#include "like_match.c"

static inline int
GenericMatchText(char *s, int slen, char* p, int plen)
{
	if (pg_database_encoding_max_length() == 1)
		return SB_MatchText(s, slen, p, plen);
	else if (GetDatabaseEncoding() == PG_UTF8)
		return UTF8_MatchText(s, slen, p, plen);
	else
		return MB_MatchText(s, slen, p, plen);
}

static inline int
Generic_Text_IC_like(text *str, text *pat)
{
	char	   *s,
			   *p;
	int			slen,
				plen;

	/* Force inputs to lower case to achieve case insensitivity */
	str = DatumGetTextP(DirectFunctionCall1(lower, PointerGetDatum(str)));
	pat = DatumGetTextP(DirectFunctionCall1(lower, PointerGetDatum(pat)));
	s = VARDATA(str);
	slen = (VARSIZE(str) - VARHDRSZ);
	p = VARDATA(pat);
	plen = (VARSIZE(pat) - VARHDRSZ);

	return GenericMatchText(s, slen, p, plen);
}

/*
 *	interface routines called by the function manager
 */

Datum
namelike(PG_FUNCTION_ARGS)
{
	Name		str = PG_GETARG_NAME(0);
	text	   *pat = PG_GETARG_TEXT_P(1);
	bool		result;
	char	   *s,
			   *p;
	int			slen,
				plen;

	s = NameStr(*str);
	slen = strlen(s);
	p = VARDATA(pat);
	plen = (VARSIZE(pat) - VARHDRSZ);

	result = (GenericMatchText(s, slen, p, plen) == LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

Datum
namenlike(PG_FUNCTION_ARGS)
{
	Name		str = PG_GETARG_NAME(0);
	text	   *pat = PG_GETARG_TEXT_P(1);
	bool		result;
	char	   *s,
			   *p;
	int			slen,
				plen;

	s = NameStr(*str);
	slen = strlen(s);
	p = VARDATA(pat);
	plen = (VARSIZE(pat) - VARHDRSZ);

	result = (GenericMatchText(s, slen, p, plen) != LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

Datum
textlike(PG_FUNCTION_ARGS)
{
	text	   *str = PG_GETARG_TEXT_P(0);
	text	   *pat = PG_GETARG_TEXT_P(1);
	bool		result;
	char	   *s,
			   *p;
	int			slen,
				plen;

	s = VARDATA(str);
	slen = (VARSIZE(str) - VARHDRSZ);
	p = VARDATA(pat);
	plen = (VARSIZE(pat) - VARHDRSZ);

	result = (GenericMatchText(s, slen, p, plen) == LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

Datum
textnlike(PG_FUNCTION_ARGS)
{
	text	   *str = PG_GETARG_TEXT_P(0);
	text	   *pat = PG_GETARG_TEXT_P(1);
	bool		result;
	char	   *s,
			   *p;
	int			slen,
				plen;

	s = VARDATA(str);
	slen = (VARSIZE(str) - VARHDRSZ);
	p = VARDATA(pat);
	plen = (VARSIZE(pat) - VARHDRSZ);

	result = (GenericMatchText(s, slen, p, plen) != LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

Datum
bytealike(PG_FUNCTION_ARGS)
{
	bytea	   *str = PG_GETARG_BYTEA_P(0);
	bytea	   *pat = PG_GETARG_BYTEA_P(1);
	bool		result;
	char	   *s,
			   *p;
	int			slen,
				plen;

	s = VARDATA(str);
	slen = (VARSIZE(str) - VARHDRSZ);
	p = VARDATA(pat);
	plen = (VARSIZE(pat) - VARHDRSZ);

	result = (SB_MatchText(s, slen, p, plen) == LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

Datum
byteanlike(PG_FUNCTION_ARGS)
{
	bytea	   *str = PG_GETARG_BYTEA_P(0);
	bytea	   *pat = PG_GETARG_BYTEA_P(1);
	bool		result;
	char	   *s,
			   *p;
	int			slen,
				plen;

	s = VARDATA(str);
	slen = (VARSIZE(str) - VARHDRSZ);
	p = VARDATA(pat);
	plen = (VARSIZE(pat) - VARHDRSZ);

	result = (SB_MatchText(s, slen, p, plen) != LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

/*
 * Case-insensitive versions
 */

Datum
nameiclike(PG_FUNCTION_ARGS)
{
	Name		str = PG_GETARG_NAME(0);
	text	   *pat = PG_GETARG_TEXT_P(1);
	bool		result;
	text	   *strtext;

	strtext = DatumGetTextP(DirectFunctionCall1(name_text,
													NameGetDatum(str)));
	result = (Generic_Text_IC_like(strtext, pat) == LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

Datum
nameicnlike(PG_FUNCTION_ARGS)
{
	Name		str = PG_GETARG_NAME(0);
	text	   *pat = PG_GETARG_TEXT_P(1);
	bool		result;
	text	   *strtext;

	strtext = DatumGetTextP(DirectFunctionCall1(name_text,
													NameGetDatum(str)));
	result = (Generic_Text_IC_like(strtext, pat) != LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

Datum
texticlike(PG_FUNCTION_ARGS)
{
	text	   *str = PG_GETARG_TEXT_P(0);
	text	   *pat = PG_GETARG_TEXT_P(1);
	bool		result;

	result = (Generic_Text_IC_like(str, pat) == LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

Datum
texticnlike(PG_FUNCTION_ARGS)
{
	text	   *str = PG_GETARG_TEXT_P(0);
	text	   *pat = PG_GETARG_TEXT_P(1);
	bool		result;

	result = (Generic_Text_IC_like(str, pat) != LIKE_TRUE);

	PG_RETURN_BOOL(result);
}

/*
 * like_escape() --- given a pattern and an ESCAPE string,
 * convert the pattern to use Postgres' standard backslash escape convention.
 */
Datum
like_escape(PG_FUNCTION_ARGS)
{
	text	   *pat = PG_GETARG_TEXT_P(0);
	text	   *esc = PG_GETARG_TEXT_P(1);
	text	   *result;

	if (pg_database_encoding_max_length() == 1)
		result = SB_do_like_escape(pat, esc);
	else
		result = MB_do_like_escape(pat, esc);

	PG_RETURN_TEXT_P(result);
}

/*
 * like_escape_bytea() --- given a pattern and an ESCAPE string,
 * convert the pattern to use Postgres' standard backslash escape convention.
 */
Datum
like_escape_bytea(PG_FUNCTION_ARGS)
{
	bytea	   *pat = PG_GETARG_BYTEA_P(0);
	bytea	   *esc = PG_GETARG_BYTEA_P(1);
	bytea	   *result = SB_do_like_escape((text *)pat, (text *)esc);

	PG_RETURN_BYTEA_P((bytea *)result);
}

