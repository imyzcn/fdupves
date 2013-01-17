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
/* @CFILE fdini.c
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2013/01/16 12:03:42 Alf*/

#include "fdini.h"

#include <glib.h>

fdini_t *g_ini;

static void
fdini_set_default (fdini_t *ini)
{
  size_t i;

  const gchar *const isuffix[] =
    {
      ".bmp",
      ".gif",
      ".jpeg",
      ".jpg",
      ".png",
      ".pcx",
      ".pnm",
      ".tif",
      ".tga",
      ".xpm",
      ".ico",
      ".cur",
      ".ani",
    };
  const gchar *const vsuffix[] =
    {
      ".avi",
      ".rmvb",
      ".rm",
      ".mp4",
      ".mkv",
      ".3gp",
      ".flv",
      ".vob",
      ".wmv",
      ".mov",
      ".swf",
    };

  if (ini == NULL)
    {
      ini = g_ini;
    }

  g_return_if_fail (ini);

  ini->verbose = FALSE;
  ini->mplayer = g_strdup ("/usr/bin/mplayer");

  ini->proc_image = FALSE;
  for (i = 0; i < sizeof (isuffix) / sizeof (isuffix[0]); ++ i)
    {
      ini->image_suffix[i] = g_strdup (isuffix[i]);
    }

  ini->proc_video = TRUE;
  for (i = 0; i < sizeof (vsuffix) / sizeof (vsuffix[0]); ++ i)
    {
      ini->video_suffix[i] = g_strdup (vsuffix[i]);
    }

  ini->video_compare_cnt = 4;

  ini->hash_distance = 2;

  ini->hash_size[0] = 8;
  ini->hash_size[1] = 8;

  ini->thumb_size[0] = 512;
  ini->thumb_size[1] = 384;

  ini->video_timers[0][0] = 5; ini->video_timers[0][1] = 15; ini->video_timers[0][2] = 1;
  ini->video_timers[1][0] = 10; ini->video_timers[1][1] = 90; ini->video_timers[1][2] = 3;
  ini->video_timers[2][0] = 60; ini->video_timers[2][1] = 600; ini->video_timers[2][2] = 10;
  ini->video_timers[3][0] = 480; ini->video_timers[3][1] = 2400; ini->video_timers[3][2] = 60;
  ini->video_timers[4][0] = 1800;  ini->video_timers[4][1] = 28800;  ini->video_timers[4][2] = 5120;
}

fdini_t *
fdini_load (const gchar *file)
{
  fdini_t *ini;

  ini = g_malloc0 (sizeof (fdini_t));
  g_return_val_if_fail (ini, NULL);

  fdini_set_default (ini);

  if (g_ini == NULL)
    {
      g_ini = ini;
    }

  return ini;

}

gboolean
fdini_save (fdini_t *ini, const gchar *file)
{
  return TRUE;

}
