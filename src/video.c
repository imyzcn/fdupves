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
/* @CFILE video.c
 *
 *  Author: Alf <naihe2010@126.com>
 */

#include "video.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <glib.h>

video_info *
video_get_info (const char *file)
{
  video_info *info;
  AVFormatContext *fmt_ctx = NULL;
  AVStream *stream = NULL;
  int i, s, ret;

  ret = avformat_open_input (&fmt_ctx, file, NULL, NULL);
  if (ret != 0)
    {
      return NULL;
    }

  for(i=0, s = -1; i < (int) fmt_ctx->nb_streams; i++)
    {
      if(fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
	  s = i;
	  break;
        }
    }

  if(s == -1)
    {
      avformat_close_input (&fmt_ctx);
      return NULL;
    }

  stream = fmt_ctx->streams[i];

  info = g_malloc0 (sizeof (video_info));

  info->name = g_path_get_basename (file);
  info->dir = g_path_get_dirname (file);
  info->length = (double) (stream->duration * stream->time_base.num) / stream->time_base.den;
  info->size[0] = stream->codec->width;
  info->size[1] = stream->codec->height;
  info->format = avcodec_get_name (stream->codec->codec_id);

  avformat_close_input (&fmt_ctx);

  return info;
}

void
video_info_free (video_info *info)
{
  g_free (info->name);
  g_free (info->dir);
  g_free (info);
}

int
video_get_length (const char *file)
{
  video_info *info;
  int length;

  length = 0;
  info = video_get_info (file);
  if (info)
    {
      length = (int) info->length;
      video_info_free (info);
    }

  return length;
}

int
video_time_screenshot (const char *file, int time,
		       int width, int height,
		       char *buffer, int buf_len)
{
  AVFormatContext *format_ctx = NULL;
  AVCodecContext *codec_ctx = NULL;
  AVCodec *codec = NULL;
  AVFrame *frame,*frame_rgb;
  AVPacket packet;
  int s, i, bytes, finished;
  int64_t seek_target;

  if (avformat_open_input (&format_ctx, file, NULL, NULL) != 0)
    {
      return -1;
    }

  s = -1;
  for (i=0; i < (int) format_ctx->nb_streams; i++)
    {
      if(format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
	  s = i;
	  break;
        }
    }

  if (s == -1)
    {
      avformat_close_input (&format_ctx);
      return -1;
    }

  codec_ctx = format_ctx->streams[s]->codec;

  codec = avcodec_find_decoder (codec_ctx->codec_id);

  if (codec == NULL)
    {
      avformat_close_input (&format_ctx);
      g_error ("Unsupported codec!");
      return -1;
    }

  if (avcodec_open2 (codec_ctx, codec, NULL) < 0)
    {
      avformat_close_input (&format_ctx);
      return -1;
    }

  frame = avcodec_alloc_frame ();
  if (frame == NULL)
    {
      avcodec_close (codec_ctx);
      avformat_close_input (&format_ctx);
      return -1;
    }
  frame_rgb = avcodec_alloc_frame ();
  if(frame_rgb == NULL)
    {
      avcodec_free_frame (&frame);
      avcodec_close (codec_ctx);
      avformat_close_input (&format_ctx);
      return -1;
    }

  bytes = avpicture_get_size (PIX_FMT_RGB24, width, height);
  if (buf_len < bytes)
    {
      avcodec_free_frame (&frame);
      avcodec_free_frame (&frame_rgb);
      avcodec_close (codec_ctx);
      avformat_close_input (&format_ctx);
      return -1;
    }
  avpicture_fill ((AVPicture *) frame_rgb, (uint8_t *) buffer,
		  PIX_FMT_RGB24,
		  width, height);

  seek_target = av_rescale (time,
			    format_ctx->streams[s]->time_base.den,
			    format_ctx->streams[s]->time_base.num);
  avformat_seek_file (format_ctx, s,
		      0, seek_target, seek_target,
		      AVSEEK_FLAG_FRAME);

  while (av_read_frame (format_ctx, &packet) >= 0)
    {
      if (packet.stream_index == s)
	{
	  avcodec_decode_video2 (codec_ctx, frame, &finished, &packet);
	  if (finished) {
	    struct SwsContext *img_convert_ctx = NULL;
	    img_convert_ctx =
	      sws_getCachedContext (img_convert_ctx,
				    codec_ctx->width, codec_ctx->height,
				    codec_ctx->pix_fmt,
				    width, height,
				    PIX_FMT_RGB24, SWS_FAST_BILINEAR,
				    NULL, NULL, NULL);
	    if (!img_convert_ctx) {
	      g_error ("Cannot initialize sws conversion context");
	      av_free_packet(&packet);
	      avcodec_free_frame (&frame);
	      avcodec_free_frame (&frame_rgb);
	      avcodec_close (codec_ctx);
	      avformat_close_input (&format_ctx);
	      return -1;
	    }
	    sws_scale (img_convert_ctx,
		       (const uint8_t * const *) frame->data, frame->linesize,
		       0, codec_ctx->height,
		       frame_rgb->data, frame_rgb->linesize);
	    sws_freeContext (img_convert_ctx);
	    av_free_packet(&packet);
	    break;
	  } /* end if (finished) */
	} /* end if (packet.stream_index */

      av_free_packet(&packet);
    } /* end while */

  av_free(frame_rgb);
  av_free(frame);

  avcodec_close(codec_ctx);

  avformat_close_input (&format_ctx);

  return bytes;
}

int
video_time_screenshot_file (const char *file, int time,
			    int width, int height,
			    const char *out_file)
{
  char *buf;
  int len;
  GdkPixbuf *pix;
  GError *err;

  buf = g_malloc (width * height * 3);
  g_return_val_if_fail (buf, -1);

  len = video_time_screenshot (file, time, width, height, buf, width * height * 3);
  if (len <= 0)
    {
      g_free (buf);
      return -1;
    }

  pix = gdk_pixbuf_new_from_data ((guchar *) buf,
				  GDK_COLORSPACE_RGB,
				  FALSE,
				  8,
				  width,
				  height,
				  width * 3,
				  NULL,
				  NULL);
  if (pix)
    {
      err = NULL;
      gdk_pixbuf_save (pix, out_file, "jpeg", &err, "quality", "100", NULL);
      if (err)
	{
	  g_free (buf);
	  g_error ("%s: %s", file, err->message);
	  g_error_free (err);
	  return -1;
	}
    }
  g_free (buf);

  return 0;
}
