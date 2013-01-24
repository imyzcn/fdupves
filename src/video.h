/*
 * This file is part of the fdvupes package
 * Copyright (C) <2008> Alf
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
/* @CFILE video.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _FDUPVES_VIDEO_H_
#define _FDUPVES_VIDEO_H_

typedef struct
{
  /* filename */
  char *name;

  /* dirname */
  char *dir;

  /* Format */
  const char *format;

  /* Duration */
  double length;

  /* Size */
  int size[2];
} video_info;

video_info * video_get_info (const char *file);

void video_info_free (video_info *info);

int video_get_length (const char *file);

int video_time_screenshot (const char *file, int time,
			   int width, int height,
			   char *buffer, int buf_len);

int video_time_screenshot_file (const char *file, int time,
				int width, int height,
				const char *out_file);

#endif
