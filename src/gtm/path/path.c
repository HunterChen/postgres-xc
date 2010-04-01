/*-------------------------------------------------------------------------
 *
 * path.c
 *	  portable path handling routines
 *
 * Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2010 Nippon Telegraph and Telephone Corporation
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL$
 *
 *-------------------------------------------------------------------------
 */

#include "gtm/gtm_c.h"

#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#include <gtm/path.h>

#define IS_DIR_SEP(ch)	((ch) == '/' || (ch) == '\\')

#define skip_drive(path)	(path)

static void trim_directory(char *path);
static void trim_trailing_separator(char *path);

/*
 *	Clean up path by:
 *		o  remove trailing slash
 *		o  remove duplicate adjacent separators
 *		o  remove trailing '.'
 *		o  process trailing '..' ourselves
 */
void
canonicalize_path(char *path)
{
	char	   *p,
			   *to_p;
	char	   *spath;
	bool		was_sep = false;
	int			pending_strips;

	/*
	 * Removing the trailing slash on a path means we never get ugly double
	 * trailing slashes. Also, Win32 can't stat() a directory with a trailing
	 * slash. Don't remove a leading slash, though.
	 */
	trim_trailing_separator(path);

	/*
	 * Remove duplicate adjacent separators
	 */
	p = path;

	to_p = p;
	for (; *p; p++, to_p++)
	{
		/* Handle many adjacent slashes, like "/a///b" */
		while (*p == '/' && was_sep)
			p++;
		if (to_p != p)
			*to_p = *p;
		was_sep = (*p == '/');
	}
	*to_p = '\0';

	/*
	 * Remove any trailing uses of "." and process ".." ourselves
	 *
	 * Note that "/../.." should reduce to just "/", while "../.." has to be
	 * kept as-is.	In the latter case we put back mistakenly trimmed ".."
	 * components below.  Also note that we want a Windows drive spec to be
	 * visible to trim_directory(), but it's not part of the logic that's
	 * looking at the name components; hence distinction between path and
	 * spath.
	 */
	spath = skip_drive(path);
	pending_strips = 0;
	for (;;)
	{
		int			len = strlen(spath);

		if (len >= 2 && strcmp(spath + len - 2, "/.") == 0)
			trim_directory(path);
		else if (strcmp(spath, ".") == 0)
		{
			/* Want to leave "." alone, but "./.." has to become ".." */
			if (pending_strips > 0)
				*spath = '\0';
			break;
		}
		else if ((len >= 3 && strcmp(spath + len - 3, "/..") == 0) ||
				 strcmp(spath, "..") == 0)
		{
			trim_directory(path);
			pending_strips++;
		}
		else if (pending_strips > 0 && *spath != '\0')
		{
			/* trim a regular directory name cancelled by ".." */
			trim_directory(path);
			pending_strips--;
			/* foo/.. should become ".", not empty */
			if (*spath == '\0')
				strcpy(spath, ".");
		}
		else
			break;
	}

	if (pending_strips > 0)
	{
		/*
		 * We could only get here if path is now totally empty (other than a
		 * possible drive specifier on Windows). We have to put back one or
		 * more ".."'s that we took off.
		 */
		while (--pending_strips > 0)
			strcat(path, "../");
		strcat(path, "..");
	}
}

/*
 *	trim_directory
 *
 *	Trim trailing directory from path, that is, remove any trailing slashes,
 *	the last pathname component, and the slash just ahead of it --- but never
 *	remove a leading slash.
 */
static void
trim_directory(char *path)
{
	char	   *p;

	path = skip_drive(path);

	if (path[0] == '\0')
		return;

	/* back up over trailing slash(es) */
	for (p = path + strlen(path) - 1; IS_DIR_SEP(*p) && p > path; p--)
		;
	/* back up over directory name */
	for (; !IS_DIR_SEP(*p) && p > path; p--)
		;
	/* if multiple slashes before directory name, remove 'em all */
	for (; p > path && IS_DIR_SEP(*(p - 1)); p--)
		;
	/* don't erase a leading slash */
	if (p == path && IS_DIR_SEP(*p))
		p++;
	*p = '\0';
}

/*
 *	trim_trailing_separator
 *
 * trim off trailing slashes, but not a leading slash
 */
static void
trim_trailing_separator(char *path)
{
	char	   *p;

	path = skip_drive(path);
	p = path + strlen(path);
	if (p > path)
		for (p--; p > path && IS_DIR_SEP(*p); p--)
			*p = '\0';
}
