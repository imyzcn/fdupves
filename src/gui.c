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
/* @CFILE gui.c
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2013/01/16 13:36:10 Alf*/

#include "util.h"
#include "gui.h"

#include <gtk/gtk.h>

typedef struct
{
  GtkWidget *widget;
  GtkWidget *mainvbox;

  GtkWidget *blogview;
  GtkWidget *articleview;
} gui_t;

static void toolbar_new (gui_t *);
static void mainframe_new (gui_t *);
static void statusbar_new (gui_t *);

static void gui_open_cb (GtkWidget *, gui_t *);
static void gui_add_cb (GtkWidget *, gui_t *);
static void gui_help_cb (GtkWidget *, gui_t *);

static gui_t gui[1];
static void gui_destroy_dialog ();
static void gui_destroy ();

gboolean
gui_init ()
{
  gui->widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (gui->widget), "fdupves 0.0.1");
  gtk_window_set_default_size (GTK_WINDOW (gui->widget), 800, 600);

  g_signal_connect (G_OBJECT (gui->widget), "destroy-event", G_CALLBACK (gui_destroy_dialog), gui);
  g_signal_connect (G_OBJECT (gui->widget), "delete-event", G_CALLBACK (gui_destroy_dialog), gui);

  gui->mainvbox = gtk_vbox_new (FALSE, FALSE);
  gtk_container_add (GTK_CONTAINER (gui->widget), gui->mainvbox);

  toolbar_new (gui);
  mainframe_new (gui);
  statusbar_new (gui);

  gtk_widget_show_all (gui->widget);

  return TRUE;
}

static void
toolbar_new (gui_t *gui)
{
  GtkWidget *toolbar;
  GtkWidget *img;
  GtkToolItem *but;

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
  gtk_box_pack_start (GTK_BOX (gui->mainvbox), toolbar, FALSE, FALSE, 2);

  img = am_toolbar_icon_new ("open.png");
  but = gtk_tool_button_new (img, _ ("Open"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (but), _ ("Open a article"));
  g_signal_connect (G_OBJECT (but), "clicked", G_CALLBACK (gui_open_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), but, -1);

  img = am_toolbar_icon_new ("pub.png");
  but = gtk_tool_button_new (img, _ ("Pub"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (but), _ ("Publish this article"));
  //  g_signal_connect (G_OBJECT (but), "clicked", G_CALLBACK (gui_pub_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), but, -1);

  img = am_toolbar_icon_new ("add.png");
  but = gtk_tool_button_new (img, _ ("Add"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (but), _ ("Add a blog"));
  g_signal_connect (G_OBJECT (but), "clicked", G_CALLBACK (gui_add_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), but, -1);

  img = am_toolbar_icon_new ("help.png");
  but = gtk_tool_button_new (img, _ ("Help"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (but), _ ("Help infomations"));
  g_signal_connect (G_OBJECT (but), "clicked", G_CALLBACK (gui_help_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), but, -1);
}

static void
mainframe_new (gui_t *gui)
{
  GtkWidget *hpaned, *lbox, *rbox;
  hpaned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (gui->mainvbox), hpaned, TRUE, TRUE, 2);

  lbox = gtk_vbox_new (FALSE, FALSE);
  gtk_paned_add1 (GTK_PANED (hpaned), lbox);

  rbox = gtk_vbox_new (FALSE, FALSE);
  gtk_paned_add2 (GTK_PANED (hpaned), rbox);

  //  gui->blogview = blog_view_new ();
  gtk_box_pack_start (GTK_BOX (lbox), gui->blogview, TRUE, TRUE, 2);

  //  gui->articleview = article_view_new ();
  gtk_box_pack_start (GTK_BOX (rbox), gui->articleview, TRUE, TRUE, 2);
}

static void
statusbar_new (gui_t *gui)
{
  GtkWidget *status;

  status = gtk_statusbar_new ();
  gtk_box_pack_end (GTK_BOX (gui->mainvbox), status, FALSE, FALSE, 2);
}

static void
gui_open_cb (GtkWidget *wid, gui_t *gui)
{
  GtkWidget *dia;

  dia = gtk_file_chooser_dialog_new (_ ("Choose Article"),
                                     GTK_WINDOW (gui->widget),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     GTK_STOCK_OK, GTK_RESPONSE_OK,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     NULL
				     );
  if (gtk_dialog_run (GTK_DIALOG (dia)) == GTK_RESPONSE_OK)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dia));
      if (filename)
        {
	  //          article_view_open (ARTICLE_VIEW (gui->articleview), filename);
          g_free (filename);
        }
    }
  gtk_widget_destroy (dia);
}

static void
add_combo_changed (GtkWidget *combo, GtkWidget *scrwin)
{
  GtkWidget *wid;
  GList *clist;
  gint id;

  id = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
  clist = gtk_container_get_children (GTK_CONTAINER (scrwin));
  //  if (clist)
  {
    //      gtk_container_remove (GTK_CONTAINER (scrwin), clist->data);
  }
  //  wid = blog_plugin_add_widget (id);
  //  gtk_container_add (GTK_CONTAINER (scrwin), wid);
  gtk_widget_show_all (scrwin);
}

static void
gui_add_cb (GtkWidget *wid, gui_t *gui)
{
  GtkWidget *dia, *vbox, *combo, *frame, *hbox, *label, *entry;
  const gchar *name;
  gsize i, size, id;
  GList *clist;

  //  size = blog_plugin_count ();
  g_return_if_fail (size > 0);

  dia = gtk_dialog_new_with_buttons (_ ("Add Blog"),
                                     GTK_WINDOW (gui->widget),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     NULL
				     );

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (dia));

  hbox = gtk_hbox_new (FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  label = gtk_label_new (_ ("Display Name"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 2);

  combo = gtk_combo_box_new_text ();
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 2);

  for (i = 0; i < size; ++ i)
    {
      //      name = blog_plugin_name (i);
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo), name);
    }

  frame = gtk_frame_new ("-");
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 2);

  g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (add_combo_changed), frame);
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

  gtk_widget_show_all (vbox);

  if (gtk_dialog_run (GTK_DIALOG (dia)) == GTK_RESPONSE_APPLY)
    {
      id = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
      clist = gtk_container_get_children (GTK_CONTAINER (frame));
      wid = clist->data;
      name = gtk_entry_get_text (GTK_ENTRY (entry));
      if (name)
        {
	  //          blog_plugin_add (id, wid, name);
        }
    }
  gtk_widget_destroy (dia);
}

static void
gui_help_cb (GtkWidget *wid, gui_t *gui)
{
}

static void
gui_destroy_dialog ()
{
  GtkWidget *dia;
  gint ret;

  dia = gtk_message_dialog_new (GTK_WINDOW (gui->widget),
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_MESSAGE_INFO,
                                GTK_BUTTONS_YES_NO,
                                _ ("Are you sure you want to quit fdupves? ")
				);

  ret = gtk_dialog_run (GTK_DIALOG (dia));
  gtk_widget_destroy (dia);

  if (ret == GTK_RESPONSE_YES)
    {
      gui_destroy ();
    }
}

static void
gui_destroy ()
{
  gtk_widget_destroy (gui->widget);
  gtk_main_quit ();
}
