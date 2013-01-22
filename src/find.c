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

#include "find.h"
#include "hash.h"
#include "video.h"
#include "ini.h"

struct st_hash
{
  int seek;
  hash_t hash;
  struct st_hash *next;
};

struct st_file
{
  const char *file;
  int length;
  struct st_hash *head_hash;
  struct st_hash *tail_hash;
};

struct st_find
{
  GPtrArray *ptr[0x10];
};

static void vfind_prepare (const gchar *, struct st_find *);
static int vfind_time_hash (struct st_file *, int, int);
static void st_file_free (struct st_file *);

void
find_images (GPtrArray *ptr, find_result_cb cb, gpointer arg)
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
		  FD_SAME_IMAGE,
		  arg);
	    }
	}
    }

  g_free (hashs);
}

void
find_videos (GPtrArray *ptr, find_result_cb cb, gpointer arg)
{
  gsize i, j, g, group_cnt, dist;
  struct st_find find[1];
  struct st_file *afile, *bfile;

  for (i = 0; g_ini->video_timers[i][0]; ++ i)
    {
      find->ptr[i] = g_ptr_array_new_with_free_func ((GFreeFunc) st_file_free);
    }
  group_cnt = i;
  g_ptr_array_foreach (ptr, (GFunc) vfind_prepare, find);

  for (g = 0; g < group_cnt; ++ g)
    {
      for (i = 0; i < find->ptr[g]->len - 1; ++ i)
	{
	  for (j = i + 1; j < find->ptr[g]->len; ++ j)
	    {
	      afile = g_ptr_array_index (find->ptr[g], i);
	      bfile = g_ptr_array_index (find->ptr[g], j);

	      if (afile->head_hash == NULL)
		{
		  vfind_time_hash (afile,
				   g_ini->video_timers[g][2],
				   0);
		}
	      if (bfile->head_hash == NULL)
		{
		  vfind_time_hash (bfile,
				   g_ini->video_timers[g][2],
				   0);
		}
	      dist = hash_cmp (afile->head_hash->hash,
			       bfile->head_hash->hash);
	      if (dist < g_ini->hash_distance)
		{
		  cb (afile->file, bfile->file,
		      FD_SAME_VIDEO_HEAD, arg);
		  continue;
		}

	      if (afile->tail_hash == NULL)
		{
		  vfind_time_hash (afile,
				   afile->length - g_ini->video_timers[g][2],
				   1);
		}
	      if (bfile->tail_hash == NULL)
		{
		  vfind_time_hash (bfile,
				   bfile->length - g_ini->video_timers[g][2],
				   1);
		}
	      dist = hash_cmp (afile->tail_hash->hash,
			       bfile->tail_hash->hash);
	      if (dist < g_ini->hash_distance)
		{
		  cb (afile->file, bfile->file,
		      FD_SAME_VIDEO_TAIL, arg);
		}
	    }
	}

      g_ptr_array_free (find->ptr[g], TRUE);
    }
}

static void
st_file_free (struct st_file *file)
{
  struct st_hash *hash;

  for (hash = file->head_hash;
       hash;
       hash = file->head_hash)
    {
      file->head_hash = hash->next;
      g_free (hash);
    }

  for (hash = file->tail_hash;
       hash;
       hash = file->tail_hash)
    {
      file->tail_hash = hash->next;
      g_free (hash);
    }

  g_free (file);
}

static void
vfind_prepare (const gchar *file, struct st_find *find)
{
  int i, length;
  struct st_file *stv;

  length = video_get_length (file);
  if (length <= 0)
    {
      g_error ("Can't get duration of %s", file);
      return;
    }

  for (i = 0; g_ini->video_timers[i][0]; ++ i)
    {
      if (length < g_ini->video_timers[i][0]
	  || length > g_ini->video_timers[i][1])
	{
	  continue;
	}

      stv = g_malloc0 (sizeof (struct st_file));

      stv->file = file;
      stv->length = length;

      g_ptr_array_add (find->ptr[i], stv);
    }
}

static int
vfind_time_hash (struct st_file *file, int seek, int tail)
{
  struct st_hash *h;

  h = g_malloc (sizeof (struct st_hash));
  h->seek = seek;
  h->hash = video_time_hash (file->file, h->seek);
  if (tail)
    {
      h->next = file->tail_hash;
      file->tail_hash = h;
    }
  else
    {
      h->next = file->head_hash;
      file->head_hash = h;
    }

  return 0;
}
