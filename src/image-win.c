/*
 * This file is part of the fdupves package
 * Copyright (C) <2012> Alf
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

#include "image-win.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <windows.h>
#ifndef COBJMACROS
#define  COBJMACROS
#endif
#include <wincodec.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int GetStride (int width, int bitCount);

GdkPixbuf *
gdk_pixbuf_new_from_file_at_scale_wic (const gchar *filename,
				       gint width, gint height,
				       gboolean keep,
				       GError **pe)
{
  GdkPixbuf *pixbuf;
  char buffer[0x1000];
  wchar_t wfilename[PATH_MAX];
  HRESULT hr;
  IWICImagingFactory *m_pIWICFactory = NULL;
  IWICBitmapDecoder *pIDecoder = NULL;
  IWICBitmapFrameDecode *pIDecoderFrame  = NULL;
  IWICBitmapScaler *pIScaler = NULL;
  WICRect rect[1];

  // Create WIC factory
  hr = CoCreateInstance(
			&CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			&IID_IWICImagingFactory,
			(LPVOID*) &m_pIWICFactory);
  if (!SUCCEEDED(hr))
    {
      if (pe)
	{
	  *pe = g_error_new (g_quark_from_string ("Warning"), hr, "Create WICImagingFactory failed: %x: %s", hr, strerror (hr));
	}

      return NULL;
    }

  MultiByteToWideChar (CP_UTF8, 0, filename, -1, wfilename, sizeof wfilename);

  hr = IWICImagingFactory_CreateDecoderFromFilename(
						    m_pIWICFactory,
						    wfilename,
						    NULL,
						    GENERIC_READ,
						    WICDecodeMetadataCacheOnDemand,
						    &pIDecoder
						    );
  if (!SUCCEEDED(hr))
    {
      if (pe)
	{
	  *pe = g_error_new (g_quark_from_string ("Warning"), hr, "Create Decoder failed");
	}
      IWICImagingFactory_Release (m_pIWICFactory);
      return NULL;
    }

  // Retrieve the first bitmap frame.
  hr = IWICBitmapDecoder_GetFrame(pIDecoder, 0, &pIDecoderFrame);

  if (!SUCCEEDED(hr))
    {
      if (pe)
	{
	  *pe = g_error_new (g_quark_from_string ("Warning"), hr, "Retrieve frame failed");
	}

      IWICBitmapDecoder_Release (pIDecoder);
      IWICImagingFactory_Release (m_pIWICFactory);

      return NULL;
    }

  // Create the scaler.
  hr = IWICImagingFactory_CreateBitmapScaler(m_pIWICFactory, &pIScaler);

  if (!SUCCEEDED(hr))
    {
      if (pe)
	{
	  *pe = g_error_new (g_quark_from_string ("Warning"), hr, "Create scaler failed");
	}

      IWICBitmapFrameDecode_Release (pIDecoderFrame);
      IWICBitmapDecoder_Release (pIDecoder);
      IWICImagingFactory_Release (m_pIWICFactory);

      return NULL;
    }

  // Initialize the scaler to width/height
  hr = IWICBitmapScaler_Initialize(
				   pIScaler,
				   (IWICBitmapSource *) pIDecoderFrame,
				   width,
				   height,
				   WICBitmapInterpolationModeFant);
  if (!SUCCEEDED (hr))
    {
      if (pe)
	{
	  *pe = g_error_new (g_quark_from_string ("Warning"), hr, "Scale failed");
	}

      IWICBitmapScaler_Release (pIScaler);
      IWICBitmapFrameDecode_Release (pIDecoderFrame);
      IWICBitmapDecoder_Release (pIDecoder);
      IWICImagingFactory_Release (m_pIWICFactory);

      return NULL;
    }

  rect->Width = width;
  rect->Height = height;
  rect->X = 0;
  rect->Y = 0;
  IWICBitmapFrameDecode_CopyPixels (
				    pIDecoderFrame,
				    rect,
				    GetStride (width, 32),
				    sizeof buffer,
				    buffer);

  pixbuf = gdk_pixbuf_new_from_data (
				     (const guchar *) buffer,
				     GDK_COLORSPACE_RGB,
				     FALSE,
				     8,
				     width,
				     height,
				     width * 4,
				     NULL,
				     pe);

  IWICBitmapScaler_Release (pIScaler);
  IWICBitmapFrameDecode_Release (pIDecoderFrame);
  IWICBitmapDecoder_Release (pIDecoder);
  IWICImagingFactory_Release (m_pIWICFactory);

  return pixbuf;
}

static int
GetStride (int width, int bitCount)
{
  int byteCount, stride;

  byteCount = bitCount / 8;
  stride = (width * byteCount + 3) & ~3;
  return stride;
}
