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
/* @CFILE main.c
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2013/01/16 10:12:33 Alf*/

#include "gui.h"
#include "ini.h"
#include "util.h"

#include <libavformat/avformat.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <curl/curl.h>
#include <locale.h>

#if defined WIN32 && defined NDEBUG
#pragma comment (linker, "/subsystem:windows")
#pragma comment (linker, "/ENTRY:mainCRTStartup")
#endif

#ifdef FDUPVES_ENABLE_PROFILER
#include <google/profiler.h>
#endif

int
main (int argc, char *argv[])
{
  gchar *prgdir, *localedir;

  prgdir = fd_install_path ();
  if (prgdir)
    {
      localedir = g_build_filename (prgdir, FD_SYS_LOCALE_DIR, NULL);
      g_free (prgdir);
    }
  else
    {
      localedir = g_strdup (FD_SYS_LOCALE_DIR);
    }

  if (localedir)
    {
      setlocale (LC_ALL, "");
      bindtextdomain (PACKAGE, localedir);
      bind_textdomain_codeset (PACKAGE, "UTF-8");
      textdomain (PACKAGE);
      g_free (localedir);
    }

  /* av format init */
  av_register_all ();

  gtk_init (&argc, &argv);

  gui_init (argc, argv);

#ifdef FDUPVES_ENABLE_PROFILER
  ProfilerStart ("fdupves.prof");
#endif
  gtk_main ();
#ifdef FDUPVES_ENABLE_PROFILER
  ProfilerStop ();
#endif

  return 0;
}
