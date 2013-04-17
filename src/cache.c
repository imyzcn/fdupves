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
/* @CFILE cache.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "cache.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

cache_t *g_cache;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef WIN32
#define strtouq _strtoui64
#endif

#ifndef FDUPVES_KEY_SEPS
#define FDUPVES_KEY_SEPS "||||"
#endif

#ifndef FDUPVES_HASH_SEPS
#define FDUPVES_HASH_SEPS "::::"
#endif

static inline void
join_key (char *key, size_t len, const char *file, int off, int alg)
{
  g_snprintf (key, len, "%s"FDUPVES_KEY_SEPS"%05d"FDUPVES_KEY_SEPS"%s", file, off, hash_phrase[alg]);
}

static inline void
split_key (const char *key, char *file, int *off, char *alg)
{
  char buf[PATH_MAX], *p, *e;

  g_snprintf (buf, sizeof buf, "%s", key);

  p = buf;
  e = strstr (p, FDUPVES_KEY_SEPS);
  g_return_if_fail (e);
  *e = '\0';
  strcpy (file, p);

  p = e + 4;
  e = strstr (p, FDUPVES_KEY_SEPS);
  g_return_if_fail (e);
  *e = '\0';
  *off = atoi (p);

  p = e + 4;
  strcpy (alg, p);
}

struct cache_s
{
  gchar *file;

  gint count;

  GHashTable *table;

  GStringChunk *chunk;
};

struct cache_value_node
{
  int time;
  int alg;
  hash_t hash;
};

struct cache_value
{
  cache_t *cache;
  gchar *file;
  GPtrArray *hashs;
};

static struct cache_value * cache_value_new (cache_t *, const char *);
static gboolean cache_value_set (struct cache_value *, int, int, hash_t);
static gboolean cache_value_get (struct cache_value *, int, int, hash_t *);
static void cache_value_free (struct cache_value *);

static gboolean write_hash (guint, struct cache_value *, FILE *);
static gboolean read_hash (char *, int *, int *, hash_t *, FILE *);

cache_t *
cache_new (const gchar *file)
{
  cache_t *cache;

  cache = g_malloc (sizeof (cache_t));
  g_return_val_if_fail (cache, NULL);

  cache->table = g_hash_table_new_full (g_str_hash,
					g_str_equal,
					NULL,
					(GDestroyNotify) cache_value_free);
  if (cache->table == NULL)
    {
      g_free (cache);
      return NULL;
    }

  cache->chunk = g_string_chunk_new (PATH_MAX * 1024 * 10);

  cache_load (cache, file);

  if (g_cache == NULL)
    {
      g_cache = cache;
    }

  return cache;
}

void
cache_free (cache_t *cache)
{
  g_string_chunk_free (cache->chunk);
  g_hash_table_destroy (cache->table);
  g_free (cache);
}

gboolean
cache_load (cache_t *cache, const char *filename)
{
  FILE *fp;
  gchar line[PATH_MAX], file[PATH_MAX];
  int off, alg;
  hash_t value[1];
  gchar *localfile;

  localfile = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
  g_return_val_if_fail (localfile, FALSE);

  fp = fopen (localfile, "rb");
  if (fp == NULL)
    {
      g_warning ("Open cache file: %s failed:%s.", localfile, strerror (errno));
      g_free (localfile);
      return FALSE;
    }

  //strip the version line. now this is not used.
  fgets (line, sizeof line, fp);

  while (read_hash (file, &off, &alg, value, fp))
    {
      if (g_file_test (file, G_FILE_TEST_IS_REGULAR))
	{
	  cache_set (cache, file, off, alg, *value);
	}
    }

  fclose (fp);

  g_free (localfile);

  return TRUE;
}

gboolean
cache_has (cache_t *cache, const gchar *file, int off, int alg)
{
  char buf[PATH_MAX];
  gpointer v;

  join_key (buf, sizeof buf, file, off, alg);
  v = g_hash_table_lookup (cache->table, buf);
  if (v == NULL)
    {
      return FALSE;
    }
  return TRUE;
}

gboolean
cache_get (cache_t *cache, const gchar *file, int off, int alg, hash_t *hp)
{
  struct cache_value *value;
  gboolean ret;

  value = g_hash_table_lookup (cache->table, file);
  if (value == NULL)
    {
      return FALSE;
    }

  ret = cache_value_get (value, off, alg, hp);
  return ret;
}

gboolean
cache_set (cache_t *cache, const gchar *file, int off, int alg, hash_t h)
{
  struct cache_value *value;
  gboolean ret;

  value = g_hash_table_lookup (cache->table, file);
  if (value == NULL)
    {
      value = cache_value_new (cache, file);
      if (value)
	{
	  g_hash_table_insert (cache->table, value->file, value);
	}
    }

  g_return_val_if_fail (value, FALSE);

  ret = cache_value_set (value, off, alg, h);
  g_return_val_if_fail (ret, FALSE);

  return TRUE;
}

gboolean
cache_remove (cache_t *cache, const gchar *file)
{
  g_hash_table_remove (cache->table, file);
  return TRUE;
}

gboolean
cache_save (cache_t *cache, const gchar *file)
{
  FILE *fp;
  char *localfile;
  char *dirname;

  if (file == NULL)
    {
      file = cache->file;
    }

  localfile = g_locale_from_utf8 (file, -1, NULL, NULL, NULL);
  g_return_val_if_fail (localfile, FALSE);

  dirname = g_path_get_dirname (file);
  if (dirname)
    {
      g_mkdir_with_parents (dirname, 0755);
      g_free (dirname);
    }

  fp = fopen (localfile, "wb");
  if (fp == NULL)
    {
      g_warning ("Open cache file: %s failed: %s", file, strerror (errno));
      g_free (localfile);
      return FALSE;
    }

  fprintf (fp, "ver:%s-%s-%s\n", PROJECT_MAJOR, PROJECT_MINOR, PROJECT_PATCH);

  g_hash_table_foreach (cache->table, (GHFunc) (write_hash), fp);

  fclose (fp);

  g_free (localfile);

  return TRUE;
}

static gboolean
write_hash (guint k, struct cache_value *value, FILE *fp)
{
  size_t i;
  struct cache_value_node *n;
  char key[PATH_MAX];

  for (i = 0; i < value->hashs->len; ++ i)
    {
      n = g_ptr_array_index (value->hashs, i);
      join_key (key, sizeof key, value->file, n->time, n->alg);
      fprintf (fp, "%s"FDUPVES_HASH_SEPS"%llu\n", key, n->hash);
    }

  return TRUE;
}

static gboolean
read_hash (char *file, int *off, int *alg, hash_t *h, FILE *fp)
{
  char buf[PATH_MAX * 2], algs[0x10], *p;
  size_t i;

  if (fgets (buf, sizeof buf, fp) == NULL)
    {
      return FALSE;
    }

  p = strstr (buf, FDUPVES_HASH_SEPS);
  if (p == NULL)
    {
      return FALSE;
    }

  *h = strtouq (p + 4, NULL, 10);

  *p = '\0';
  split_key (buf, file, off, algs);

  for (i = 0; i < FDUPVES_HASH_ALGS_CNT; ++ i)
    {
      if (strcmp (algs, hash_phrase[i]) == 0)
	{
	  *alg = i;
	  break;
	}
    }

  return TRUE;
}

static struct cache_value *
cache_value_new (cache_t *cache, const char *file)
{
  struct cache_value *value;

  value = g_malloc (sizeof (struct cache_value));
  g_return_val_if_fail (value, NULL);

  value->cache = cache;
  value->file = g_string_chunk_insert_const (cache->chunk, file);
  value->hashs = g_ptr_array_new_with_free_func (g_free);

  return value;
}

static gboolean
cache_value_set (struct cache_value *value, int time, int alg, hash_t h)
{
  size_t i;
  struct cache_value_node *n;

  for (i = 0; i < value->hashs->len; ++ i)
    {
      n = g_ptr_array_index (value->hashs, i);
      if (n->time == time
	  && n->alg == alg)
	{
	  n->hash = h;
	  return TRUE;
	}
    }

  n = g_malloc (sizeof (struct cache_value_node));
  g_return_val_if_fail (n, FALSE);

  n->time = time;
  n->alg = alg;
  n->hash = h;
  g_ptr_array_add (value->hashs, n);

  return TRUE;
}

static gboolean
cache_value_get (struct cache_value *value, int time, int alg, hash_t *hp)
{
  size_t i;
  struct cache_value_node *n;

  for (i = 0; i < value->hashs->len; ++ i)
    {
      n = g_ptr_array_index (value->hashs, i);
      if (n->time == time
	  && n->alg == alg)
	{
	  *hp = n->hash;
	  return TRUE;
	}
    }

  return FALSE;
}

static void
cache_value_free (struct cache_value *value)
{
  g_ptr_array_free (value->hashs, TRUE);
  g_free (value);
}
