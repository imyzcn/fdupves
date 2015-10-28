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
/* @CFILE ini.c
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2013/01/16 12:03:42 Alf*/

#include "ini.h"
#include "util.h"

#include <glib.h>

ini_t *g_ini;

static void ini_free (ini_t *);

ini_t *
ini_new ()
{
  ini_t *ini;
  size_t i;

  const gchar *const isuffix[] =
    {
      ".bmp",
      ".gif",
      ".jpeg",
      ".jpg",
      ".jpe",
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
      ".mp4",
      ".mpg",
      ".rmvb",
      ".rm",
      ".mov",
      ".mkv",
      ".m4v",
      ".mpg",
      ".mpeg",
      ".vob",
      ".asf",
      ".wmv",
      ".3gp",
      ".flv",
      ".mod",
      ".swf",
      ".mts",
    };

  ini = g_malloc0 (sizeof (ini_t));
  g_return_val_if_fail (ini, NULL);

  if (g_ini == NULL)
    {
      g_ini = ini;
    }

  ini->keyfile = g_key_file_new ();

  ini->verbose = FALSE;

  ini->proc_image = FALSE;
  for (i = 0; i < sizeof (isuffix) / sizeof (isuffix[0]); ++ i)
    {
      g_snprintf (ini->image_suffix[i],
		  sizeof (ini->image_suffix[i]), "%s",
		  isuffix[i]);
    }
  ini->image_suffix[i][0] = 0;

  ini->proc_video = TRUE;
  for (i = 0; i < sizeof (vsuffix) / sizeof (vsuffix[0]); ++ i)
    {
      g_snprintf (ini->video_suffix[i],
		  sizeof (ini->video_suffix[i]), "%s",
		  vsuffix[i]);
    }
  ini->video_suffix[i][0] = 0;

  ini->proc_other = FALSE;

  ini->compare_area = 0;

  ini->compare_count = 4;

  ini->same_image_distance = 5;
  ini->same_video_distance = 5;

  ini->thumb_size[0] = 512;
  ini->thumb_size[1] = 384;

  ini->video_timers[0][0] = 10;
  ini->video_timers[0][1] = 120;
  ini->video_timers[0][2] = 4;
  ini->video_timers[1][0] = 60;
  ini->video_timers[1][1] = 600;
  ini->video_timers[1][2] = 25;
  ini->video_timers[2][0] = 300;
  ini->video_timers[2][1] = 3000;
  ini->video_timers[2][2] = 120;
  ini->video_timers[3][0] = 1500;
  ini->video_timers[3][1] = 28800;
  ini->video_timers[3][2] = 600;
  ini->video_timers[4][0] = 0;

  ini->cache_file = g_build_filename (g_get_home_dir (),
				      ".cache",
				      "fdupves",
				      NULL);

  return ini;
}

ini_t *
ini_new_with_file (const gchar *file)
{
  ini_t *ini;

  ini = ini_new ();
  g_return_val_if_fail (ini, NULL);

  if (ini_load (ini, file) == FALSE)
    {
      ini_free (ini);
      return NULL;
    }

  return ini;
}

gboolean
ini_load (ini_t *ini, const gchar *file)
{
  gchar *path;
  GError *err;

  path = fd_realpath (file);
  g_return_val_if_fail (path, FALSE);

  err = NULL;
  g_key_file_load_from_file (ini->keyfile, path, 0, &err);
  g_free (path);
  if (err)
    {
      g_warning ("load configuration file: %s failed: %s", file, err->message);
      g_error_free (err);
      return FALSE;
    }

  if (g_key_file_has_key (ini->keyfile, "_", "proc_image", NULL))
    {
      ini->proc_image = g_key_file_get_boolean (ini->keyfile,
						"_",
						"proc_image",
						NULL);
    }
  if (g_key_file_has_key (ini->keyfile, "_", "proc_video", NULL))
    {
      ini->proc_video = g_key_file_get_boolean (ini->keyfile,
						"_",
						"proc_video",
						NULL);
    }

  if (g_key_file_has_key (ini->keyfile, "_", "compare_area", NULL))
    {
      ini->compare_area = g_key_file_get_integer (ini->keyfile,
                                                  "_",
                                                  "compare_area",
                                                  NULL);
    }

  if (g_key_file_has_key (ini->keyfile, "_", "compare_count", NULL))
    {
      ini->compare_count = g_key_file_get_integer (ini->keyfile,
						   "_",
						   "compare_count",
						   NULL);
    }

  return TRUE;
}

gboolean
ini_save (ini_t *ini, const gchar *file)
{
  gchar *data, *path;
  gsize len;

  path = fd_realpath (file);
  g_return_val_if_fail (path, FALSE);

  g_key_file_set_boolean (ini->keyfile, "_", "proc_image", ini->proc_image);
  g_key_file_set_boolean (ini->keyfile, "_", "proc_video", ini->proc_video);
  g_key_file_set_integer (ini->keyfile, "_", "compare_area", ini->compare_area);
  g_key_file_set_integer (ini->keyfile, "_", "compare_count", ini->compare_count);

  data = g_key_file_to_data (ini->keyfile, &len, NULL);
  g_file_set_contents (path, data, len, NULL);
  g_free (path);
  g_free (data);

  return TRUE;
}

static
void ini_free (ini_t *ini)
{
  if (g_ini == ini)
    {
      g_ini = NULL;
    }

  g_key_file_free (ini->keyfile);
  g_free (ini);
}
