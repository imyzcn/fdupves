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
/* @CFILE phash.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "hash.h"
#include "util.h"
#include "video.h"
#include "image.h"

#include <glib.h>
#include <math.h>

#define FDUPVES_PHASH_LEN 32
#define FDUPVES_DCT_LEN 8

static hash_t pixbuf_phash (GdkPixbuf *);
static gboolean buffer_dct (const unsigned char *,
			    unsigned char *, gsize);
static const gdouble *get_coefficient ();
static const gdouble *get_coefficient_t ();
static void matrix_mul (const gdouble *, const gdouble *,
			gdouble *);

hash_t
file_phash (const char *file)
{
  GdkPixbuf *buf;
  hash_t h;
  GError *err;

  err = NULL;
  buf = fdupves_gdkpixbuf_load_file_at_size (file,
					     FDUPVES_PHASH_LEN,
					     FDUPVES_PHASH_LEN,
					     &err);

  if (err)
    {
      g_warning ("Load file: %s to pixbuf failed: %s", file, err->message);
      g_error_free (err);
      return 0;
    }

  h = pixbuf_phash (buf);
  g_object_unref (buf);

  return h;
}

hash_t
buffer_phash (const char *buffer, int size)
{
  GdkPixbuf *buf;
  GError *err;
  hash_t h;

  err = NULL;
  buf = gdk_pixbuf_new_from_data ((const guchar *) buffer,
				  GDK_COLORSPACE_RGB,
				  FALSE,
				  8,
				  FDUPVES_PHASH_LEN,
				  FDUPVES_PHASH_LEN,
				  FDUPVES_PHASH_LEN * 3,
				  NULL,
				  &err);
  if (err)
    {
      g_warning ("Load inline data to pixbuf failed: %s", err->message);
      g_error_free (err);
      return 0;
    }

  h = pixbuf_phash (buf);
  g_object_unref (buf);

  return h;
}

hash_t
video_time_phash (const char *file, int time)
{
  hash_t v;
  gchar buffer[FDUPVES_PHASH_LEN * FDUPVES_PHASH_LEN * 3];

  video_time_screenshot (file, time,
			 FDUPVES_PHASH_LEN,
			 FDUPVES_PHASH_LEN,
			 buffer, sizeof buffer);

  v = buffer_phash (buffer, sizeof buffer);

  return v;
}

static hash_t
pixbuf_phash (GdkPixbuf *pixbuf)
{
  int width, height, rowstride, n_channels;
  guchar *pixels, *p;
  int sum, avg, x, y, off;
  hash_t hash;
  unsigned char *grays,
    dct[FDUPVES_PHASH_LEN * FDUPVES_PHASH_LEN],
    dctc[FDUPVES_DCT_LEN * FDUPVES_DCT_LEN];

  n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
  g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  grays = g_new (unsigned char, width * height);
  off = 0;
  for (y = 0; y < width; ++ y)
    {
      for (x = 0; x < height; ++ x)
	{
	  p = pixels + y * rowstride + x * n_channels;
	  grays[off] = (p[0] * 30 + p[1] * 59 + p[2] * 11) / 100;
	  ++ off;
	}
    }

  buffer_dct (grays, dct, sizeof dct);

  sum = 0;
  off = 0;
  for (x = 0; x < FDUPVES_DCT_LEN; ++ x)
    {
      for (y = 0; y < FDUPVES_DCT_LEN; ++ y)
	{
	  sum += dct[x * FDUPVES_PHASH_LEN + y];
	  dctc[off] = dct[x * FDUPVES_PHASH_LEN + y];
	  ++ off;
	}
    }
  avg = sum / off;

  hash = 0;
  for (x = 0; x < off; ++ x)
    {
      if (dctc[x] >= avg)
	{
	  hash |= (((hash_t) 1) << x);
	}
    }

  g_free (grays);

  return hash;
}

static gboolean
buffer_dct (const unsigned char *pix, unsigned char *out_pix, gsize out_len)
{
  const gdouble *quotient, *quotientT;
  gdouble matrix[FDUPVES_PHASH_LEN * FDUPVES_PHASH_LEN],
    temp[FDUPVES_PHASH_LEN * FDUPVES_PHASH_LEN];
  gsize i, j;

  g_assert (out_len >= FDUPVES_PHASH_LEN * FDUPVES_PHASH_LEN);

  for(i = 0; i < FDUPVES_PHASH_LEN; i ++)
    {
      for(j = 0; j < FDUPVES_PHASH_LEN; j ++)
	{
	  matrix[i * FDUPVES_PHASH_LEN + j] =
	    (gdouble) (pix[i * FDUPVES_PHASH_LEN + j]);
	}
    }

  quotient = get_coefficient ();
  quotientT = get_coefficient_t ();

  matrix_mul (quotient, matrix, temp);
  matrix_mul (temp, quotientT, matrix);

  for (i = 0; i < FDUPVES_PHASH_LEN; i ++)
    {
      for (j = 0; j < FDUPVES_PHASH_LEN; j ++)
	{
	  out_pix[i * FDUPVES_PHASH_LEN + j] =
	    (unsigned char) matrix[i * FDUPVES_PHASH_LEN + j];
	}
    }

  return TRUE;
}

static const gdouble *
get_coefficient_t ()
{
  gsize i, j;
  static gdouble *coeff_s;
  const gdouble *c;

  if (coeff_s)
    {
      return coeff_s;
    }

  coeff_s = g_new (gdouble, FDUPVES_PHASH_LEN * FDUPVES_PHASH_LEN);
  g_assert (coeff_s);

  c = get_coefficient ();

  for (i = 0; i < FDUPVES_PHASH_LEN; i ++)
    {
      for (j = 0; j < FDUPVES_PHASH_LEN; j ++)
	{
	  coeff_s[i * FDUPVES_PHASH_LEN + j] =
	    c[j * FDUPVES_PHASH_LEN + i];
	}
    }

  return coeff_s;
}

static const gdouble *
get_coefficient ()
{
  gsize i, j;
  static gdouble *coeff_s = NULL;
  gdouble s;

  if (coeff_s)
    {
      return coeff_s;
    }

  coeff_s = g_new (gdouble, FDUPVES_PHASH_LEN * FDUPVES_PHASH_LEN);
  g_assert (coeff_s);

  s = 1.0 / sqrt (FDUPVES_PHASH_LEN);
  for (i = 0; i < FDUPVES_PHASH_LEN; i ++)
    {
      coeff_s[i] = s;
    }
  for (i = 1; i < FDUPVES_PHASH_LEN; i ++)
    {
      for (j = 0; j < FDUPVES_PHASH_LEN; j ++)
	{
	  coeff_s[i * FDUPVES_PHASH_LEN + j] = sqrt (2.0 / FDUPVES_PHASH_LEN)
	    * cos (i * M_PI * (j + 0.5) / (gdouble) FDUPVES_PHASH_LEN);
	}
    }

  return coeff_s;
}

static void
matrix_mul (const gdouble *A, const gdouble *B, gdouble *matrix)
{
  gdouble t;
  gsize i, j, k;

  for (i = 0; i < FDUPVES_PHASH_LEN; i ++)
    {
      for (j = 0; j < FDUPVES_PHASH_LEN; j ++)
	{
	  t = 0.0;
	  for (k = 0; k < FDUPVES_PHASH_LEN; k ++)
	    {
	      t += A[i * FDUPVES_PHASH_LEN + k]
		* B[k * FDUPVES_PHASH_LEN + j];
	    }
	  matrix[i * FDUPVES_PHASH_LEN + j] = t;
	}
    }
}
