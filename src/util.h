/*
 * This file is part of the fdupves package
 * Copyright (C) <2010>  <Alf>
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
/* @CFILE util.h
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2013/01/16 11:20:08 Alf*/

#ifndef _FDUPVES_UTIL_H_
#define _FDUPVES_UTIL_H_

#include <gtk/gtk.h>

#include <libintl.h>
#include <glib.h>

/*
 * %s indicate the prefix
 * */
#define AM_SYS_CONF_FILE   "etc/fdupvesrc"
#define AM_SYS_ICON_DIR    "share/fdupves/icons"
#define AM_SYS_LOCALE_DIR  "share/locale"
#define AM_SYS_PLUGIN_DIR  "lib/fdupves"

/*
 * ~ indicate the user home
 * */
#define AM_USR_CONF_DIR    "~/.fdupves"
#define AM_USR_CONF_FILE   "~/.fdupves/config"
#define AM_USR_PLUGIN_DIR  "~/.fdupves/plugin"


#define _(S)    gettext (S)

G_MODULE_EXPORT gchar * am_realpath (const gchar *);

G_MODULE_EXPORT gchar * am_install_path ();

GtkWidget * am_toolbar_icon_new (const gchar *);

#endif
