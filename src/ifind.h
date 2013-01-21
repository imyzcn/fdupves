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
/* @CFILE ifind.h
 *
 *  Author: Alf <naihe2010@126.com>
 */

#ifndef _FDUPVES_IFIND_H_
#define _FDUPVES_IFIND_H_

#include <glib.h>

#define FIND_OK_CB(f) (void (*) (const gchar *, const gchar *, gpointer)) (f)
typedef void (*find_ok_cb) (const gchar *, const gchar *, gpointer);

void find_images (GPtrArray *, find_ok_cb, gpointer);

void find_videos (GPtrArray *, find_ok_cb, gpointer);

#endif
