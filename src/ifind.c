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
/* @CFILE ifind.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "ifind.h"
#include "ihash.h"
#include "fdini.h"

void
find_images (GPtrArray *ptr, find_ok_cb cb, gpointer arg)
{
  size_t i, j;
  int dist;
  hash_t *hashs;

  hashs = g_new0 (hash_t, ptr->len);
  g_return_if_fail (hashs);

  for (i = 0; i < ptr->len; ++ i)
    {
      hashs[i] = file_hash ((gchar *) g_ptr_array_index (ptr, i));
    }

  for (i = 0; i < ptr->len - 1; ++ i)
    {
      for (j = i + 1; j < ptr->len; ++ j)
	{
	  dist = hash_cmp (hashs[i], hashs[j]);
	  if (dist < g_ini->hash_distance)
	    {
	      cb (g_ptr_array_index (ptr, i),
		  g_ptr_array_index (ptr, j),
		  arg);
	    }
	}
    }

  g_free (hashs);
}

void
find_videos (GPtrArray *ptr, find_ok_cb cb, gpointer arg)
{
}
