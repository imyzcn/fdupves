/*
 * This file is part of the fdupves package
 * Copyright (C) <2010>  <Alf>
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
/* @CFILE util.c
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2013/01/16 11:21:05 Alf*/

#include "util.h"
#include "ini.h"

#include <stdlib.h>
#include <limits.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX        4096
#endif

gchar *
fd_realpath (const gchar *path)
{
  gchar *abpath;
  gchar opath[PATH_MAX];

  if (g_path_is_absolute (path))
    {
      return g_strdup (path);
    }

  if (path[0] == '~')
    {
      const gchar *home = getenv ("HOME");
      if (home == NULL)
        {
          home = g_get_home_dir ();
        }
      if (home == NULL)
        {
          return NULL;
        }
      abpath = g_build_filename (home, path + 1, NULL);
    }
  else
    {
#ifdef WIN32
      getcwd (sizeof (opath), opath);
      strcat (opath, "\\");
      strcat (opath, path);
#else
      if (realpath (path, opath) == NULL)
        {
          return NULL;
        }
#endif
      abpath = g_strdup (opath);
    }

  return abpath;
}

gchar *
fd_install_path ()
{
  gchar *prgdir;

#ifdef WIN32
  prgdir = g_win32_get_package_installation_directory_of_module (NULL);
#else
  prgdir = g_strdup (CMAKE_INSTALL_PREFIX);
#endif

  return prgdir;
}

int
is_image (const char *path)
{
  int i;
  const char *p, *s;

  p = strrchr (path, '.');
  if (p == NULL)
    {
      return 0;
    }

  for (i = 0; g_ini->image_suffix[i][0]; ++ i)
    {
      s = g_ini->image_suffix[i];
      if (g_ascii_strcasecmp (p, s) == 0)
	{
	  return 1;
	}
    }

  return 0;
}

int
is_video (const gchar *path)
{
  int i;
  const char *p, *s;

  p = strrchr (path, '.');
  if (p == NULL)
    {
      return 0;
    }

  for (i = 0; g_ini->video_suffix[i][0]; ++ i)
    {
      s = g_ini->video_suffix[i];
      if (g_ascii_strcasecmp (p, s) == 0)
	{
	  return 1;
	}
    }

  return 0;
}
