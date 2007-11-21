#include <config.h>

#ifndef HAVE_BASENAME

#include <stddef.h>

#include "port.h"


#ifdef _WIN32
#define IS_DIR_SEP(ch) ((ch) == '/' || (ch) == '\\')
#else
#define IS_DIR_SEP(ch) ((ch) == '/')
#endif

char *
basename(const char *path)
{
	const char *res = path;

	if (res == NULL)
		return (char *) res;

	for (res=path; *path; ++path)
	{
		if (IS_DIR_SEP(*path))
		{
			res = path + 1;
		}
	}

	return (char *) res;
}


#endif	/* !HAVE_BASENAME */
