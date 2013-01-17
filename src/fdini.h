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
/* @CFILE fdini.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2013/01/16 11:02:08 Alf*/

#ifndef _FDUPVES_AMINI_H_
#define _FDUPVES_AMINI_H_

#include <glib.h>

typedef struct
{
  gboolean verbose;
  gchar *mplayer;

  gboolean proc_image;
  gchar *image_suffix[0x100];

  gboolean proc_video;
  gchar *video_suffix[0x100];
  gint video_compare_cnt;

  gint hash_distance;

  gint hash_size[2];

  gint thumb_size[2];

  gint video_timers[0x10][3];
} fdini_t;

extern fdini_t * g_ini;

fdini_t * fdini_load (const gchar *);

gboolean fdini_save (fdini_t *, const gchar *);

#endif
