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

#include <string.h>

#ifndef FD_VIDEO_COMP_CNT
#define FD_VIDEO_COMP_CNT 2
#endif

struct st_hash
{
  int seek;
  hash_t hash;
};

struct st_file
{
  const char *file;
  int length;
  struct st_hash head[1];
  struct st_hash tail[1];
};

struct st_find
{
  GPtrArray *ptr[0x10];
};

static void vfind_prepare (const gchar *, struct st_find *);
static int vfind_time_hash (struct st_file *, int, int);
static void st_file_free (struct st_file *);
static gboolean is_video_same (struct st_file *, struct st_file *, gboolean);
static GSList *append_same_slist (GSList *,
				  const gchar *, const gchar *,
				  same_type);

GSList *
find_images (GPtrArray *ptr)
{
  GSList *list;
  size_t i, j;
  int dist;
  hash_t *hashs;

  hashs = g_new0 (hash_t, ptr->len);
  g_return_val_if_fail (hashs, NULL);

  for (i = 0; i < ptr->len; ++ i)
    {
      hashs[i] = file_hash ((gchar *) g_ptr_array_index (ptr, i));
    }

  list = NULL;
  for (i = 0; i < ptr->len - 1; ++ i)
    {
      for (j = i + 1; j < ptr->len; ++ j)
	{
	  dist = hash_cmp (hashs[i], hashs[j]);
	  if (dist < g_ini->hash_distance)
	    {
	      list = append_same_slist (list,
					g_ptr_array_index (ptr, i),
					g_ptr_array_index (ptr, j),
					FD_SAME_IMAGE);
	    }
	}
    }

  g_free (hashs);

  return list;
}

GSList *
find_videos (GPtrArray *ptr)
{
  GSList *list;
  gsize i, j, g, group_cnt, dist;
  struct st_find find[1];
  struct st_file *afile, *bfile;

  for (i = 0; g_ini->video_timers[i][0]; ++ i)
    {
      find->ptr[i] = g_ptr_array_new_with_free_func ((GFreeFunc) st_file_free);
    }
  group_cnt = i;
  g_ptr_array_foreach (ptr, (GFunc) vfind_prepare, find);

  list = NULL;
  for (g = 0; g < group_cnt; ++ g)
    {
      for (i = 0; i < find->ptr[g]->len - 1; ++ i)
	{
	  for (j = i + 1; j < find->ptr[g]->len; ++ j)
	    {
	      afile = g_ptr_array_index (find->ptr[g], i);
	      bfile = g_ptr_array_index (find->ptr[g], j);

	      if (afile->head->hash == 0)
		{
		  vfind_time_hash (afile,
				   g_ini->video_timers[g][2],
				   0);
		}
	      if (bfile->head->hash == 0)
		{
		  vfind_time_hash (bfile,
				   g_ini->video_timers[g][2],
				   0);
		}
	      dist = hash_cmp (afile->head->hash,
			       bfile->head->hash);
	      if (dist < g_ini->hash_distance)
		{
		  if (is_video_same (afile, bfile, FALSE))
		    {
		      list = append_same_slist (list,
						afile->file, bfile->file,
						FD_SAME_VIDEO_HEAD);
		      continue;
		    }
		}

	      if (afile->tail->hash == 0)
		{
		  vfind_time_hash (afile,
				   afile->length - g_ini->video_timers[g][2],
				   1);
		}
	      if (bfile->tail->hash == 0)
		{
		  vfind_time_hash (bfile,
				   bfile->length - g_ini->video_timers[g][2],
				   1);
		}
	      dist = hash_cmp (afile->tail->hash,
			       bfile->tail->hash);
	      if (dist < g_ini->hash_distance)
		{
		  if (is_video_same (afile, bfile, TRUE))
		    {
		      list = append_same_slist (list,
						afile->file, bfile->file,
						FD_SAME_VIDEO_TAIL);
		    }
		}
	    }
	}

      g_ptr_array_free (find->ptr[g], TRUE);
    }

  return list;
}

static void
st_file_free (struct st_file *file)
{
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

void
same_node_free (same_node *node)
{
  g_slist_free (node->files);
  g_free (node);
}

void
same_list_free (GSList *list)
{
  g_slist_free_full (list, (GDestroyNotify) same_node_free);
}

static gboolean
is_video_same (struct st_file *afile, struct st_file *bfile, gboolean tail)
{
  int seeka[FD_VIDEO_COMP_CNT], seekb[FD_VIDEO_COMP_CNT];
  int i, rate, length;
  hash_t hasha, hashb;

  if (tail)
    {
      length = afile->tail->seek < bfile->tail->seek ?
	afile->tail->seek:
	bfile->tail->seek;
      rate =  length / (FD_VIDEO_COMP_CNT + 1);
      for (i = 0; i < FD_VIDEO_COMP_CNT; ++ i)
	{
	  seeka[i] = afile->tail->seek - (i + 1) * rate;
	  seekb[i] = bfile->tail->seek - (i + 1) * rate;
	}
    }
  else
    {
      length = afile->length < bfile->length ? afile->length: bfile->length;
      rate = (length - afile->head->seek) / (FD_VIDEO_COMP_CNT + 1);
      for (i = 0; i < FD_VIDEO_COMP_CNT; ++ i)
	{
	  seeka[i] = afile->head->seek + (i + 1) * rate;
	  seekb[i] = bfile->head->seek + (i + 1) * rate;
	}
    }

  for (i = 0; i < FD_VIDEO_COMP_CNT; ++ i)
    {
      hasha = video_time_hash (afile->file, seeka[i]);
      hashb = video_time_hash (bfile->file, seekb[i]);
      if (hash_cmp (hasha, hashb) >= g_ini->hash_distance)
	{
	  return FALSE;
	}
    }

  return TRUE;
}

static int
vfind_time_hash (struct st_file *file, int seek, int tail)
{
  if (tail)
    {
      file->tail->seek = seek;
      file->tail->hash = video_time_hash (file->file, seek);
    }
  else
    {
      file->head->seek = seek;
      file->head->hash = video_time_hash (file->file, seek);
    }

  return 0;
}

static GSList *
append_same_slist (GSList *slist,
		   const gchar *afile, const gchar *bfile,
		   same_type type)
{
  GSList *cur, *fslist;
  same_node *node;
  gboolean afind, bfind;

  for (cur = slist; cur; cur = g_slist_next (cur))
    {
      node = cur->data;

      if (node->type != type)
	{
	  continue;
	}

      afind = bfind = FALSE;
      for (fslist = node->files; fslist; fslist = g_slist_next (fslist))
	{
	  if (strcmp (fslist->data, afile) == 0)
	    {
	      afind = TRUE;
	    }
	  else if (strcmp (fslist->data, bfile) == 0)
	    {
	      bfind = TRUE;
	    }
	}

      if (afind && bfind)
	{
	  return slist;
	}
      else if (afind)
	{
	  node->files = g_slist_append (node->files, (gpointer) bfile);
	  return slist;
	}
      else if (bfind)
	{
	  node->files = g_slist_append (node->files, (gpointer) afile);
	  return slist;
	}
    }

  node = g_malloc0 (sizeof (same_node));
  g_return_val_if_fail (node, slist);

  node->type = type;
  node->files = g_slist_append (node->files, (gpointer) afile);
  node->files = g_slist_append (node->files, (gpointer) bfile);

  return g_slist_append (slist, node);
}
