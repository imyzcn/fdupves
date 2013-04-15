/*
 * This file is part of the fdupves package
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
/* @CFILE image.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "image.h"
#ifdef WIN32
#include "image-win.h"
#endif

GdkPixbuf *
fdupves_gdkpixbuf_load_file_at_size (const gchar *file, int w, int h, GError **error)
{
  GdkPixbuf *buf;
#ifdef WIN32
  int width, height;
  GdkPixbufFormat *format;

  width = 0;
  height = 0;

  format = gdk_pixbuf_get_file_info (file,
				     &width, &height);
  if (format == NULL)
    {
      g_warning ("Get file: %s infomation failed.", file);
    }
#endif

  if (error)
    {
      *error = NULL;
    }
#ifdef WIN32
  if (width > 2000 && height > 2000)
    {
      buf = gdk_pixbuf_new_from_file_at_scale_wic (file,
						   w,
						   h,
						   FALSE,
						   error);
    }
  else
#endif
    {
      buf = gdk_pixbuf_new_from_file_at_scale (file,
					       w,
					       h,
					       FALSE,
					       error);
    }

  return buf;
}
