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
#include "ini.h"
#include "video.h"
#include "image.h"
#include "find.h"
#include "gui.h"
#include "cache.h"

#include <glib/gstdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#endif

typedef struct
{
  same_type type;
  GSList *files;
  GtkTreeRowReference *treerowref;
  gboolean show;
} same_node;

void same_node_free (same_node *);
void same_list_free (GSList *);

typedef struct
{
  /* same node */
  same_node *node;

  gchar *path;
  gchar *name;
  gchar *dir;

  /* FD_IMAGE FD_VIDEO */
  gint type;

  /* format desc */
  gchar *format;

  /* file size */
  gint size;

  /* image/screenshot size */
  gint width, height;

  /* video length */
  gdouble length;

  /* bool select */
  gboolean selected;
} file_node;

static file_node * file_node_new (same_node *,
				  const gchar *, gint);
static void file_node_free (file_node *);
/* free the file node and same_node if necesstity */
static void file_node_free_full (file_node *);
static void file_node_to_tree_iter (file_node *,
				    GtkTreeStore *, GtkTreeIter *);

typedef struct
{
  GtkWidget *widget;
  GtkWidget *mainvbox;
  GtkWidget *progress;

  GtkToolItem *but_add;
  GtkToolItem *but_find;
  GtkToolItem *but_del;

  GtkListStore *dirliststore;

  GPtrArray *images;
  GPtrArray *videos;
  GSList *same_images;
  GSList *same_videos;
  GSList *same_list;

  GtkWidget *logtree;
  GtkListStore *logliststore;

  GtkWidget *restree;
  GtkTreeStore *restreestore;
  GtkTreeSelection *resselect;
  file_node **resselfiles;
} gui_t;

typedef struct
{
  GtkWidget *dialog;

  GtkWidget *content;
  GtkWidget *container;

  GtkWidget *butprev, *butnext;

  const file_node *afn, *bfn;

  gboolean from_tail;

  gint index;

  gint alen;
  gint blen;

  gui_t *gui;
} diff_dialog;

static void diff_dialog_new (gui_t *, const file_node *, const file_node *);
static void diffdia_onclose (GtkWidget *, diff_dialog *);
static void diffdia_onresponse (GtkWidget *, gint, diff_dialog *);
static void diffdia_onseekchanged (GtkEntry *, diff_dialog *);
static void diffdia_onheadtail (GtkToggleButton *, diff_dialog *);
static void diffdia_onnext (GtkWidget *, diff_dialog *);
static void diffdia_onprev (GtkWidget *, diff_dialog *);

static void toolbar_new (gui_t *);
static void mainframe_new (gui_t *);
static void progressbar_new (gui_t *);

static void gui_add_cb (GtkWidget *, gui_t *);
static void gui_find_cb (GtkWidget *, gui_t *);
static void gui_delsel_cb (GtkWidget *, gui_t *);
static void gui_pref_cb (GtkWidget *, gui_t *);
static void gui_help_cb (GtkWidget *, gui_t *);

static void gui_find_step_cb (const find_step *, gui_t *);
static GSList *gui_append_same_slist (gui_t *, GSList *,
				      const gchar *, const gchar *,
				      same_type);

static gui_t gui[1];
static void gui_destroy (gui_t *);
static void gui_destroy_cb (GtkWidget *, GdkEvent *, gui_t *);
static void gui_add_dir (gui_t *, const gchar *);

static void gui_find_thread (gui_t *);

static gboolean dir_find_item (GtkTreeModel *,
			       GtkTreePath *,
			       GtkTreeIter *,
			       gui_t *);
static void gui_list_dir (gui_t *, const gchar *);
static void gui_list_file (gui_t *, const gchar *);
static void gui_list_link (gui_t *, const gchar *);

static void restree_sel_small_file (same_node *node, gui_t *);
static void restree_sel_big_file (same_node *node, gui_t *);
static void restree_sel_small_image (same_node *node, gui_t *);
static void restree_sel_big_image (same_node *node, gui_t *);
static void restree_sel_short_video (same_node *node, gui_t *);
static void restree_sel_long_video (same_node *node, gui_t *);
static void restree_sel_others (same_node *node, gui_t *);

static void restree_filter_changed (GtkEntry *, gui_t *);
static void restree_filter_focusin (GtkEntry *, gui_t *);
static void restree_selcombo_changed (GtkComboBox *, gui_t *);
static void dirlist_onactivated (GtkTreeView *,
				 GtkTreePath *,
				 GtkTreeViewColumn *,
				 gui_t *);
static gboolean restree_onbutpress (GtkWidget *,
				    GdkEventButton *,
				    gui_t *);
static void restree_onactivated (GtkTreeView *,
				 GtkTreePath *,
				 GtkTreeViewColumn *,
				 gui_t *);
static void restreesel_onchanged (GtkTreeSelection *, gui_t *);

static GtkWidget *restree_open_menuitem (gui_t *);
static GtkWidget *restree_opendir_menuitem (gui_t *);
static GtkWidget *restree_delete_menuitem (gui_t *);
static GtkWidget *restree_diff_menuitem (gui_t *);

static void restree_open (GtkMenuItem *, gui_t *);
static void restree_opendir (GtkMenuItem *, gui_t *);
static void restree_delete (GtkMenuItem *, gui_t *);
static void restree_diff (GtkMenuItem *, gui_t *);

#ifndef FDUPVES_THREAD_STACK_SIZE
#define FDUPVES_THREAD_STACK_SIZE (1024 * 1024 * 10)
#endif

#ifndef FDUPVES_MAXLOG
#define FDUPVES_MAXLOG 1000
#endif

gboolean
gui_init (int argc, char *argv[])
{
  int i;
  gchar *insdir, *iconfile;

  if (ini_new_with_file (FD_USR_CONF_FILE) == FALSE)
    {
      ini_new ();
    }

  gui->widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (gui->widget), PACKAGE_STRING);
  gtk_window_set_default_size (GTK_WINDOW (gui->widget), 800, 600);

  insdir = fd_install_path ();
  if (insdir)
    {
      iconfile = g_build_filename (insdir, FD_SYS_ICON_DIR, PROJECT_ICON, NULL);
      gtk_window_set_icon_from_file (GTK_WINDOW (gui->widget), iconfile, NULL);
      g_free (iconfile);
      g_free (insdir);
    }

  g_signal_connect (G_OBJECT (gui->widget), "destroy-event", G_CALLBACK (gui_destroy_cb), gui);
  g_signal_connect (G_OBJECT (gui->widget), "delete-event", G_CALLBACK (gui_destroy_cb), gui);

  gui->mainvbox = gtk_vbox_new (FALSE, FALSE);
  gtk_container_add (GTK_CONTAINER (gui->widget), gui->mainvbox);

  toolbar_new (gui);
  mainframe_new (gui);
  progressbar_new (gui);

  gtk_widget_show_all (gui->widget);

  for (i = 1; i < argc; ++ i)
    {
      gui_add_dir (gui, argv[i]);
    }

  return TRUE;
}

static GtkWidget *
fd_toolbar_icon_new (const gchar *name)
{
  gchar *prgdir, *icondir, *iconfile;
  GtkWidget *img;

  img = NULL;

  prgdir = fd_install_path ();
  if (prgdir)
    {
      icondir = g_build_filename (prgdir, FD_SYS_ICON_DIR, NULL);
      if (icondir)
        {
          iconfile = g_build_filename (icondir, name, NULL);
          if (iconfile)
            {
              img = gtk_image_new_from_file (iconfile);
              g_free (iconfile);
            }

          g_free (icondir);
        }

      g_free (prgdir);
    }

  return img;
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

  img = fd_toolbar_icon_new ("add.png");
  gui->but_add = gtk_tool_button_new (img, _ ("Add"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (gui->but_add),
			       _ ("Add Source Files"));
  g_signal_connect (G_OBJECT (gui->but_add),
		    "clicked",
		    G_CALLBACK (gui_add_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gui->but_add, -1);

  img = fd_toolbar_icon_new ("find.png");
  gui->but_find = gtk_tool_button_new (img, _ ("Find"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (gui->but_find),
			       _ ("Find Duplicate Files"));
  g_signal_connect (G_OBJECT (gui->but_find),
		    "clicked",
		    G_CALLBACK (gui_find_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gui->but_find, -1);

  img = fd_toolbar_icon_new ("del.png");
  gui->but_del = gtk_tool_button_new (img, _ ("Delete Selected"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (gui->but_del),
			       _ ("Delete the selected files"));
  g_signal_connect (G_OBJECT (gui->but_del),
		    "clicked",
		    G_CALLBACK (gui_delsel_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gui->but_del, -1);

  img = fd_toolbar_icon_new ("pref.png");
  but = gtk_tool_button_new (img, _ ("Preference"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (but),
			       _ ("Preference"));
  g_signal_connect (G_OBJECT (but),
		    "clicked",
		    G_CALLBACK (gui_pref_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), but, -1);

  img = fd_toolbar_icon_new ("help.png");
  but = gtk_tool_button_new (img, _ ("Help"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (but), _ ("Help infomations"));
  g_signal_connect (G_OBJECT (but), "clicked", G_CALLBACK (gui_help_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), but, -1);
}

static void
gui_log (const gchar *log_domain,
	 GLogLevelFlags log_level,
	 const gchar *message,
	 gpointer user_data)
{
  GtkTreeIter itr[1];
  GtkTreePath *path;
  gui_t *gui;

  gui = (gui_t *) user_data;

  gdk_threads_enter ();
  gtk_list_store_append (gui->logliststore, itr);
  gtk_list_store_set (gui->logliststore, itr, 0, message, -1);

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (gui->logliststore),
				  itr);
  gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (gui->logtree),
				path, NULL,
				FALSE,
				0.0, 0.0);
  gtk_tree_path_free (path);

  if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (gui->logliststore),
				      NULL) >= FDUPVES_MAXLOG)
    {
      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gui->logliststore),
				     itr);
      gtk_list_store_remove (gui->logliststore, itr);
    }
  gdk_threads_leave ();
}

static void
gui_ibutcb (GtkWidget *but, gui_t *gui)
{
  g_ini->proc_image = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON (but));
}

static void
gui_vbutcb (GtkWidget *but, gui_t *gui)
{
  g_ini->proc_video = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON (but));
}

static void
gui_compareareacb(GtkWidget *combo, gui_t *gui)
{
  g_ini->compare_area = gtk_combo_box_get_active
    (GTK_COMBO_BOX (combo));
}

static void
mainframe_new (gui_t *gui)
{
  GtkWidget *hpaned, *vpaned, *vbox, *hbox, *dirview, *win;
  GtkWidget *typebox, *typeibut, *typevbut;
  GtkWidget *entry, *combo, *comparearea;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  hpaned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (gui->mainvbox), hpaned, TRUE, TRUE, 2);

  vpaned = gtk_vpaned_new ();
  gtk_paned_add1 (GTK_PANED (hpaned), vpaned);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_paned_add1 (GTK_PANED (vpaned), vbox);

  /* dir list win */
  win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
				       GTK_SHADOW_IN);
  gui->dirliststore = gtk_list_store_new (1, G_TYPE_STRING);
  dirview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gui->dirliststore));
  gtk_container_add (GTK_CONTAINER (win), dirview);
  gtk_widget_set_size_request (dirview, 300, 200);
  gtk_box_pack_start (GTK_BOX (vbox), win, TRUE, TRUE, 2);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_ ("Source Dir"),
     renderer, "text", 0,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dirview), column);

  g_signal_connect (G_OBJECT (dirview), "row-activated", G_CALLBACK (dirlist_onactivated), gui);

  typebox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_end (GTK_BOX (vbox), typebox, FALSE, FALSE, 2);

  typeibut = gtk_check_button_new_with_label (_ ("Image"));
  gtk_box_pack_start (GTK_BOX (typebox), typeibut, FALSE, FALSE, 2);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (typeibut), g_ini->proc_image);
  g_signal_connect (G_OBJECT (typeibut), "toggled", G_CALLBACK (gui_ibutcb), gui);

  typevbut = gtk_check_button_new_with_label (_ ("Video"));
  gtk_box_pack_start (GTK_BOX (typebox), typevbut, FALSE, FALSE, 2);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (typevbut), g_ini->proc_video);
  g_signal_connect (G_OBJECT (typevbut), "toggled", G_CALLBACK (gui_vbutcb), gui);

  typebox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_end (GTK_BOX (vbox), typebox, FALSE, FALSE, 2);

  comparearea = gtk_combo_box_text_new ();
  gtk_box_pack_start (GTK_BOX (typebox), comparearea, FALSE, FALSE, 2);
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (comparearea), _ ("Compare all"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (comparearea), _ ("Compare top"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (comparearea), _ ("Compare bottom"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (comparearea), _ ("Compare left"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (comparearea), _ ("Compare right"));
  g_signal_connect (G_OBJECT (comparearea), "changed", G_CALLBACK (gui_compareareacb), gui);
  gtk_combo_box_set_active (GTK_COMBO_BOX (comparearea), g_ini->compare_area);

  /* log win */
  win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
				       GTK_SHADOW_IN);
  gui->logliststore = gtk_list_store_new (1, G_TYPE_STRING);
  gui->logtree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gui->logliststore));
  gtk_container_add (GTK_CONTAINER (win), gui->logtree);
  gtk_paned_add2 (GTK_PANED (vpaned), win);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_ ("Message"),
     renderer, "text", 0,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (gui->logtree), column);

  g_log_set_handler (NULL, G_LOG_LEVEL_MASK,
		     gui_log, gui);

  /* result area */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_paned_add2 (GTK_PANED (hpaned), vbox);

  /* result filter */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 2);
  gtk_entry_set_text (GTK_ENTRY (entry), _ ("filename filter condition"));
  g_signal_connect (G_OBJECT (entry), "changed",
		    G_CALLBACK (restree_filter_changed), gui);
  g_signal_connect (G_OBJECT (entry), "focus-in-event",
		    G_CALLBACK (restree_filter_focusin), gui);

  combo = gtk_combo_box_new_text ();
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 2);
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _ ("Select manual"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _ ("select small file"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _ ("select big file"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _ ("select small image"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _ ("select big image"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _ ("select short video"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _ ("select long video"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _ ("select others"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  g_signal_connect (G_OBJECT (combo), "changed",
		    G_CALLBACK (restree_selcombo_changed), gui);

  /* result tree */
  win = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), win, TRUE, TRUE, 2);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
				       GTK_SHADOW_IN);
  gui->restreestore = gtk_tree_store_new (6,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_POINTER
					  );
  gui->restree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gui->restreestore));
  gtk_container_add (GTK_CONTAINER (win), gui->restree);

  /* path */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_ ("File Path"),
     renderer, "text", 0,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (gui->restree), column);
  /* image size */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_ ("Image Size"),
     renderer, "text", 1,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (gui->restree), column);
  /* file size */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_ ("File Size"),
     renderer, "text", 2,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (gui->restree), column);
  /* video length */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_ ("Video Length"),
     renderer, "text", 3,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (gui->restree), column);
  /* format */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_ ("Format"),
     renderer, "text", 4,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (gui->restree), column);

  gtk_widget_add_events (GTK_WIDGET (gui->restree), GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT (gui->restree), "button-press-event",
		    G_CALLBACK (restree_onbutpress), gui);
  g_signal_connect (G_OBJECT (gui->restree), "row-activated",
		    G_CALLBACK (restree_onactivated), gui);
  gui->resselect = gtk_tree_view_get_selection (GTK_TREE_VIEW (gui->restree));
  gtk_tree_selection_set_mode (gui->resselect, GTK_SELECTION_MULTIPLE);
  g_signal_connect (G_OBJECT (gui->resselect), "changed",
		    G_CALLBACK (restreesel_onchanged), gui);
}

static void
progressbar_new (gui_t *gui)
{
  gui->progress = gtk_progress_bar_new ();
  gtk_box_pack_end (GTK_BOX (gui->mainvbox), gui->progress, FALSE, FALSE, 2);
}

static void
gui_add_dir (gui_t *gui, const char *path)
{
  GtkTreeIter itr[1];

  gtk_list_store_append (gui->dirliststore, itr);
  gtk_list_store_set (gui->dirliststore, itr, 0, path, -1);
}

static void
gui_add_cb (GtkWidget *wid, gui_t *gui)
{
  GtkWidget *dia;

  dia = gtk_file_chooser_dialog_new (_ ("Choose Folder"),
                                     GTK_WINDOW (gui->widget),
                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                     GTK_STOCK_OK, GTK_RESPONSE_OK,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     NULL
				     );
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dia),
					TRUE);

  if (gtk_dialog_run (GTK_DIALOG (dia)) == GTK_RESPONSE_OK)
    {
      GSList *list, *cur;

      list = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dia));
      for (cur = list; cur; cur = g_slist_next (cur))
        {
	  gui_add_dir (gui, cur->data);
	  g_free (cur->data);
        }
      g_slist_free (list);
    }
  gtk_widget_destroy (dia);
}

static void
gui_find_cb (GtkWidget *wid, gui_t *gui)
{
  g_thread_create_full ((GThreadFunc) gui_find_thread,
			gui,
			FDUPVES_THREAD_STACK_SIZE,
			FALSE,
			FALSE,
			0,
			NULL);
}

static void
gui_find_thread (gui_t *gui)
{
  int fimage, fvideo;

  /* disable the add/find tool time */
  gdk_threads_enter ();
  gtk_widget_set_sensitive (GTK_WIDGET (gui->but_add), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gui->but_find), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gui->but_del), FALSE);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (gui->progress), "");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gui->progress), 0);

  gtk_tree_store_clear (gui->restreestore);
  if (gui->same_list)
    {
      same_list_free (gui->same_list);
      gui->same_list = NULL;
    }
  gdk_threads_leave ();

  gui->images = g_ptr_array_new_with_free_func (g_free);
  gui->videos = g_ptr_array_new_with_free_func (g_free);

  gtk_tree_model_foreach (GTK_TREE_MODEL (gui->dirliststore),
			  (GtkTreeModelForeachFunc) dir_find_item,
			  gui);

  fimage = 0;
  if (g_ini->proc_image && gui->images->len > 0)
    {
      g_message (_ ("find %d images to process"), gui->images->len);

      find_images (gui->images, (find_step_cb) gui_find_step_cb, gui);
      fimage = g_slist_length (gui->same_list);
      g_message (_ ("find %d groups same images"), fimage);
    }

  fvideo = 0;
  if (g_ini->proc_video && gui->videos->len > 0)
    {
      g_message (_ ("find %d videos to process"), gui->videos->len);

      find_videos (gui->videos, (find_step_cb) gui_find_step_cb, gui);
      fvideo = g_slist_length (gui->same_list);
      fvideo -= fimage;
      g_message (_ ("find %d groups same videos"), fvideo);
    }

  g_ptr_array_free (gui->images, TRUE);
  g_ptr_array_free (gui->videos, TRUE);

  gdk_threads_enter ();
  gtk_tree_view_expand_all (GTK_TREE_VIEW (gui->restree));

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gui->progress), 0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (gui->progress), "");

  /* disable the add/find tool time */
  gtk_widget_set_sensitive (GTK_WIDGET (gui->but_add), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (gui->but_find), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (gui->but_del), TRUE);
  gdk_threads_leave ();
}

static void
gui_pref_cb (GtkWidget *wid, gui_t *gui)
{
}

static void
gui_help_cb (GtkWidget *wid, gui_t *gui)
{
  gchar *prgdir, *helpfile;
#ifndef WIN32
  gchar *uri;
#endif

  prgdir = fd_install_path ();
  if (prgdir)
    {
      helpfile = g_build_filename (prgdir, FD_SYS_HELP_FILE, NULL);
      g_free (prgdir);
    }
  else
    {
      helpfile = g_strdup (FD_SYS_HELP_FILE);
    }

#ifndef WIN32
  uri = g_filename_to_uri (helpfile, NULL, NULL);
  gtk_show_uri (NULL, uri, GDK_CURRENT_TIME, NULL);
  g_free (uri);
#else
  ShellExecute (NULL, "open", helpfile, NULL, NULL, SW_SHOW);
#endif

  g_free (helpfile);
}

static gboolean
dir_find_item (GtkTreeModel *model,
	       GtkTreePath *tpath,
	       GtkTreeIter *itr,
	       gui_t *gui)
{
  gchar *path;

  gtk_tree_model_get (model, itr, 0, &path, -1);
  if (path)
    {
      if (g_file_test (path, G_FILE_TEST_IS_DIR))
	{
	  gui_list_dir (gui, path);
	}
      else if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
	{
	  gui_list_file (gui, path);
	}
      else if (g_file_test (path, G_FILE_TEST_IS_SYMLINK))
	{
	  gui_list_link (gui, path);
	}

      g_free (path);
    }

  return FALSE;
}

static void
gui_list_dir (gui_t *gui, const gchar *path)
{
  GQueue stack[1];
  GDir *gdir;
  GError *err;
  gchar *dir, *curpath;
  const gchar *cur;

  g_queue_init (stack);
  dir = g_strdup (path);
  g_queue_push_tail (stack, dir);

  while ((dir = g_queue_pop_tail (stack)) != NULL)
    {
      err = NULL;
      gdir = g_dir_open (dir, 0, &err);
      if (err)
	{
	  g_warning ("Can't open dir: %s: %s", dir, err->message);
	  g_error_free (err);
	}

      while ((cur = g_dir_read_name (gdir)) != NULL)
	{
	  if (strcmp (cur, ".") == 0
	      || strcmp (cur, "..") == 0)
	    {
	      continue;
	    }

	  curpath = g_build_filename (dir, cur, NULL);
	  if (g_file_test (curpath, G_FILE_TEST_IS_DIR))
	    {
	      g_queue_push_tail (stack, curpath);
	    }
	  else if (g_file_test (curpath, G_FILE_TEST_IS_REGULAR))
	    {
	      gui_list_file (gui, curpath);
	      g_free (curpath);
	    }
	  else if (g_file_test (curpath, G_FILE_TEST_IS_SYMLINK))
	    {
	      gui_list_link (gui, curpath);
	      g_free (curpath);
	    }
	}

      g_dir_close (gdir);
      g_free (dir);
    }
}

static void
gui_list_file (gui_t *gui, const gchar *path)
{
  gchar *p;

  if (is_image (path))
    {
      if (g_ini->proc_image)
	{
	  p = g_strdup (path);
	  g_ptr_array_add (gui->images, p);
	  return;
	}
    }

  else if (is_video (path))
    {
      if (g_ini->proc_video)
	{
	  p = g_strdup (path);
	  g_ptr_array_add (gui->videos, p);
	  return;
	}
    }

  else
    {
      if (g_ini->proc_other)
	{
	}
      else
	{
	  g_debug ("%s is not image/video, skipped", path);
	}
    }
}

static void
gui_list_link (gui_t *gui, const gchar *path)
{
}

static void
gui_destroy_cb (GtkWidget *but, GdkEvent *ev, gui_t *gui)
{
  GtkWidget *dia;
  gint ret;

  dia = gtk_message_dialog_new (GTK_WINDOW (gui->widget),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_YES_NO,
				_ ("Are you sure you want to quit fdupves?")
				);

  ret = gtk_dialog_run (GTK_DIALOG (dia));
  gtk_widget_destroy (dia);

  if (ret == GTK_RESPONSE_YES)
    {
      gui_destroy (gui);
    }
}

static void
gui_destroy (gui_t *gui)
{
  ini_save (g_ini, FD_USR_CONF_FILE);

  gtk_widget_destroy (gui->widget);
  gtk_main_quit ();
}

static void
dirlist_onactivated (GtkTreeView *tree,
		     GtkTreePath *path,
		     GtkTreeViewColumn *column,
		     gui_t *gui)
{
  GtkTreeIter itr[1];

  if (gtk_tree_model_get_iter
      (GTK_TREE_MODEL (gui->dirliststore),
       itr,
       path))
    {
      gtk_list_store_remove (gui->dirliststore, itr);
    }
}

static void
restree_onactivated (GtkTreeView *tree,
		     GtkTreePath *path,
		     GtkTreeViewColumn *column,
		     gui_t *gui)
{
  restree_open (NULL, gui);
}

static gboolean
restree_onbutpress (GtkWidget *wid,
		    GdkEventButton *event,
		    gui_t *gui)
{
  GtkWidget *menu, *item;
  int selcnt;

  if (event->button != 3 || event->type != GDK_BUTTON_PRESS)
    {
      return FALSE;
    }

  selcnt = gtk_tree_selection_count_selected_rows (gui->resselect);
  if (selcnt == 0)
    {
      return FALSE;
    }

  menu = gtk_menu_new ();
  switch (selcnt)
    {
    case 1:
      item = restree_open_menuitem (gui);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      item = restree_opendir_menuitem (gui);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      item = restree_delete_menuitem (gui);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      break;

    case 2:
      item = restree_diff_menuitem (gui);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      item = restree_delete_menuitem (gui);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      break;

    default: /* >= 3 */
      item = restree_delete_menuitem (gui);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      break;
    }

  gtk_menu_set_title (GTK_MENU (menu), _ ("process file"));
  gtk_widget_show_all (menu);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
		  event->button, event->time);

  return TRUE;
}

static GtkWidget *
restree_open_menuitem (gui_t *gui)
{
  GtkWidget *item;

  item = gtk_menu_item_new_with_label (_ ("open"));
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (restree_open), gui);

  return item;
}

static GtkWidget *
restree_opendir_menuitem (gui_t *gui)
{
  GtkWidget *item;

  item = gtk_menu_item_new_with_label (_ ("open dir"));
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (restree_opendir), gui);

  return item;
}

static GtkWidget *
restree_delete_menuitem (gui_t *gui)
{
  GtkWidget *item;

  item = gtk_menu_item_new_with_label (_ ("delete"));
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (restree_delete), gui);

  return item;
}

static GtkWidget *
restree_diff_menuitem (gui_t *gui)
{
  GtkWidget *item;

  item = gtk_menu_item_new_with_label (_ ("diff"));
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (restree_diff), gui);

  return item;
}

static void
restreesel_onchanged (GtkTreeSelection *sel, gui_t *gui)
{
  GList *list, *cur;
  gsize i, cnt;
  GtkTreeIter itr[1];

  if (gui->resselfiles)
    {
      g_free (gui->resselfiles);
      gui->resselfiles = NULL;
    }

  list = gtk_tree_selection_get_selected_rows (sel, NULL);
  cnt = g_list_length (list);
  if (cnt < 1)
    {
      return;
    }

  gui->resselfiles = g_new0 (file_node *, cnt + 1);

  for (i = 0, cur = list; cur; ++ i, cur = g_list_next (cur))
    {
      gtk_tree_model_get_iter (GTK_TREE_MODEL (gui->restreestore),
			       itr,
			       cur->data);
      gtk_tree_model_get (GTK_TREE_MODEL (gui->restreestore), itr,
			  5, gui->resselfiles + i,
			  -1);
      gtk_tree_path_free (cur->data);
    }
  g_list_free (list);
}

static void
restree_open (GtkMenuItem *item, gui_t *gui)
{
#ifndef WIN32
  gchar *uri;
  GError *err;

  uri = g_filename_to_uri (gui->resselfiles[0]->path, NULL, NULL);
  err = NULL;
  gtk_show_uri (NULL, uri, GDK_CURRENT_TIME, &err);
  if (err)
    {
      g_debug ("Open uri: %s error: %s", uri, err->message);
      g_error_free (err);
    }
  g_free (uri);

#else
  gchar *filename;

  filename = g_win32_locale_filename_from_utf8 (gui->resselfiles[0]->path);
  if (filename)
    {
      ShellExecute (NULL, "open", filename, NULL, NULL, SW_SHOW);
      g_free (filename);
    }
  else
    {
      ShellExecute (NULL, "open", gui->resselfiles[0]->path, NULL, NULL, SW_SHOW);
    }
#endif
}

static void
restree_opendir (GtkMenuItem *item, gui_t *gui)
{
  gchar *dir;
#ifndef WIN32
  gchar *uri;
  GError *err;
#else
  gchar *dirname;
#endif

  dir = g_path_get_dirname (gui->resselfiles[0]->path);
  if (dir == NULL)
    {
      g_warning ("get file: %s dirname failed", gui->resselfiles[0]->path);
      return;
    }

#ifndef WIN32
  uri = g_filename_to_uri (dir, NULL, NULL);

  err = NULL;
  gtk_show_uri (NULL, uri, GDK_CURRENT_TIME, &err);
  if (err)
    {
      g_debug ("Open uri: %s error: %s", uri, err->message);
      g_error_free (err);
    }
  g_free (uri);

#else

  dirname = g_win32_locale_filename_from_utf8 (dir);
  if (dirname)
    {
      ShellExecute (NULL, "open", dirname, NULL, NULL, SW_SHOW);
      g_free (dirname);
    }
  else
    {
      ShellExecute (NULL, "open", dir, NULL, NULL, SW_SHOW);
    }
#endif

  g_free (dir);
}

static void
gui_filter_result (gui_t *gui, const gchar *filter)
{
  GtkTreeIter itr[1], itrc[1];
  GtkTreePath *path;
  GSList *nodelist, *filelist;
  same_node *node;
  file_node *fn;
  gboolean match;

  gtk_tree_store_clear (gui->restreestore);

  for (nodelist = gui->same_list;
       nodelist != NULL;
       nodelist = g_slist_next (nodelist))
    {
      node = (same_node *) nodelist->data;

      if (node->treerowref)
	{
	  gtk_tree_row_reference_free (node->treerowref);
	  node->treerowref = NULL;
	}
      node->show = FALSE;

      if (node->files == NULL)
	{
	  continue;
	}

      if (filter == NULL)
	{
	  match = TRUE;
	}
      else
	{
	  match = FALSE;

	  for (filelist = node->files;
	       filelist != NULL;
	       filelist = g_slist_next (filelist))
	    {
	      fn = filelist->data;

	      if (strstr (fn->path, filter))
		{
		  match = TRUE;
		  break;
		}
	    }
	}

      if (match)
	{
	  gtk_tree_store_append (gui->restreestore, itr, NULL);
	  fn = node->files->data;
	  file_node_to_tree_iter (fn, gui->restreestore, itr);

	  path = gtk_tree_model_get_path (GTK_TREE_MODEL (gui->restreestore),
					  itr);
	  node->treerowref = gtk_tree_row_reference_new (GTK_TREE_MODEL (gui->restreestore), path);
	  gtk_tree_path_free (path);

	  for (filelist = g_slist_next (node->files);
	       filelist != NULL;
	       filelist = g_slist_next (filelist))
	    {
	      gtk_tree_store_append (gui->restreestore, itrc, itr);
	      fn = filelist->data;
	      file_node_to_tree_iter (fn, gui->restreestore, itrc);
	    }

	  node->show = TRUE;
	}
    }

  gtk_tree_view_expand_all (GTK_TREE_VIEW (gui->restree));
}

#ifdef WIN32
static int
win32_remove (gui_t *gui, const gchar *filename, gboolean totrash)
{
  int ret;
  gchar *lname, destfile[PATH_MAX], *d;
  const gchar *p;
  SHFILEOPSTRUCT FileOp;

  lname = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
  if (lname)
    {
      for (p = lname, d = destfile;
	   *p;
	   ++ p, ++ d)
	{
	  *d = *p;
	  if (*d == '\\')
	    {
	      * (++ d) = '\\';
	    }
	}
      *d = '\0';
      *(d + 1) = '\0';
      g_free (lname);
    }
  else
    {
      for (p = filename, d = destfile;
	   *p;
	   ++ p, ++ d)
	{
	  *d = *p;
	  if (*d == '\\')
	    {
	      * (++ d) = '\\';
	    }
	}
      *d = '\0';
      *(d + 1) = '\0';
    }

  memset (&FileOp, 0, sizeof FileOp);
  FileOp.hwnd = HWND_DESKTOP;
  FileOp.wFunc = FO_DELETE;
  FileOp.fFlags = FOF_NOCONFIRMATION;
  FileOp.pFrom = destfile;
  FileOp.lpszProgressTitle = "Delete file";

  if (totrash)
    {
      FileOp.fFlags |= FOF_ALLOWUNDO;
    }

  ret = SHFileOperation (&FileOp);

  return ret;
}
#endif

#define FDUPVES_DEL_ALL 1
#define FDUPVES_DEL_TOTRASH (1<<1)

static void
gui_delete_all_cb (GtkToggleButton *but, gint *pflags)
{
  g_assert (pflags);

  if (gtk_toggle_button_get_active (but))
    {
      *pflags |= FDUPVES_DEL_ALL;
    }
  else
    {
      *pflags &= (~ FDUPVES_DEL_ALL);
    }
}

#ifdef WIN32
static void
gui_delete_totrash_cb (GtkToggleButton *but, gint *pflags)
{
  g_assert (pflags);

  if (gtk_toggle_button_get_active (but))
    {
      *pflags |= FDUPVES_DEL_TOTRASH;
    }
  else
    {
      *pflags &= (~ FDUPVES_DEL_TOTRASH);
    }
}
#endif

static gint
gui_delete_dialog_ask (gui_t *gui, const gchar *filename, gint *pflags)
{
  GtkWidget *dia, *all;
  gint res;
#ifdef WIN32
  GtkWidget *totrash;
#endif

  dia = gtk_message_dialog_new (GTK_WINDOW (gui->widget),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_YES_NO,
				_ ("Are you sure you want to delete %s?")
				, filename);
  all = gtk_check_button_new_with_label (_ ("Use this action for all the other files"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dia)->vbox), all, TRUE, TRUE, 2);
  g_signal_connect (G_OBJECT (all), "toggled",
		    G_CALLBACK (gui_delete_all_cb), pflags);
#ifdef WIN32
  totrash = gtk_check_button_new_with_label (_ ("move file to trash, but not really delete it"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dia)->vbox), totrash, TRUE, TRUE, 2);
  g_signal_connect (G_OBJECT (totrash), "toggled",
		    G_CALLBACK (gui_delete_totrash_cb), pflags);
#endif
  gtk_widget_show_all (GTK_WIDGET (GTK_DIALOG (dia)->vbox));

  res = gtk_dialog_run (GTK_DIALOG (dia));
  gtk_widget_destroy (dia);

  return res;
}

static void
restree_delete (GtkMenuItem *item, gui_t *gui)
{
  gint i, res, ret, flags;

  res = 0;
  flags = 0;
  for (i = 0; gui->resselfiles[i]; ++ i)
    {
      if (res == 0)
	{
	  res = gui_delete_dialog_ask (gui, gui->resselfiles[i]->path, &flags);
	}

      if (res == GTK_RESPONSE_YES)
	{
#ifdef WIN32
	  if (flags & FDUPVES_DEL_TOTRASH)
	    {
	      ret = win32_remove (gui, gui->resselfiles[i]->path, TRUE);
	    }
	  else
	    {
	      ret = win32_remove (gui, gui->resselfiles[i]->path, FALSE);
	    }
#else
	  ret = g_remove (gui->resselfiles[i]->path);
#endif
	  if (ret == 0)
	    {
	      file_node_free_full (gui->resselfiles[i]);
	    }
	}

      if (!(flags & FDUPVES_DEL_ALL))
	{
	  res = 0;
	}
    }

  gui_filter_result (gui, NULL);
}

static GtkWidget *
image2widget (const file_node *fn)
{
  gchar *desc;
  GtkWidget *vbox, *label, *image;
  GdkPixbuf *pixbuf;
  GError *err;

  desc = g_strdup_printf ("Name: %s\n"
			  "Dir: %s\n"
			  "size: %d:%d\n"
			  "format: %s",
			  fn->name,
			  fn->dir,
			  fn->width, fn->height,
			  fn->format);

  label = gtk_label_new (desc);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 60);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
  g_free (desc);

  err = NULL;
  pixbuf = fdupves_gdkpixbuf_load_file_at_size (fn->path,
						g_ini->thumb_size[0],
						g_ini->thumb_size[1],
						&err);
  if (err)
    {
      g_warning ("load image: %s error: %s", fn->path, err->message);
      g_error_free (err);
      g_free (desc);
      return NULL;
    }
  image = gtk_image_new_from_pixbuf (pixbuf);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (vbox), image, TRUE, FALSE, 2);

  return vbox;
}

static void
diff_add_image (diff_dialog *dia, const file_node *afn, const file_node *bfn)
{
  GtkWidget *aimage, *bimage;

  dia->container = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (dia->content), dia->container, TRUE, TRUE, 0);

  aimage = image2widget (afn);
  gtk_box_pack_start (GTK_BOX (dia->container), aimage, TRUE, TRUE, 0);

  bimage = image2widget (bfn);
  gtk_box_pack_end (GTK_BOX (dia->container), bimage, TRUE, TRUE, 0);
}

static GtkWidget *
video2widget (const file_node *fn, int seek)
{
  gchar *desc, *tmpfile, tmpname[0x10];
  GtkWidget *label, *image, *vbox;

  desc = g_strdup_printf ("Name: %s\n"
			  "Dir: %s\n"
			  "Format %s\n"
			  "Length: %f\n"
			  "Size: %d-%d",
			  fn->name,
			  fn->dir,
			  fn->format,
			  fn->length,
			  fn->width, fn->height);
  label = gtk_label_new (desc);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 60);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
  g_free (desc);

  g_snprintf (tmpname, sizeof tmpname, "%d.jpg", rand ());
  tmpfile = g_build_filename (g_get_tmp_dir (), tmpname, NULL);
  video_time_screenshot_file (fn->path, seek,
			      g_ini->thumb_size[0], g_ini->thumb_size[1],
			      tmpfile);
  image = gtk_image_new_from_file (tmpfile);
  g_free (tmpfile);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (vbox), image, TRUE, FALSE, 2);

  return vbox;
}

static void
diffdia_refresh_video_pic (diff_dialog *dia)
{
  GList *list, *cur;
  GtkWidget *avideo, *bvideo;
  int seek, rate;

  list = gtk_container_get_children (GTK_CONTAINER (dia->container));
  for (cur = list; cur; cur = g_list_next (cur))
    {
      gtk_container_remove (GTK_CONTAINER (dia->container), cur->data);
    }

  rate = (int) ((dia->afn->length < dia->bfn->length ?
		 dia->afn->length:
		 dia->bfn->length) / (g_ini->compare_count + 1));

  seek = dia->from_tail ?
    (int) (dia->afn->length - rate * dia->index) :
    rate * dia->index;
  avideo = video2widget (dia->afn, seek);
  gtk_box_pack_start (GTK_BOX (dia->container), avideo, TRUE, TRUE, 0);

  seek = dia->from_tail ?
    (int) (dia->bfn->length - rate * dia->index) :
    rate * dia->index;
  bvideo = video2widget (dia->bfn, seek);
  gtk_box_pack_end (GTK_BOX (dia->container), bvideo, TRUE, TRUE, 0);

  if (dia->index <= 1)
    {
      gtk_widget_set_sensitive (dia->butprev, FALSE);
      gtk_widget_set_sensitive (dia->butnext, TRUE);
    }
  else if (dia->index >= g_ini->compare_count)
    {
      gtk_widget_set_sensitive (dia->butprev, TRUE);
      gtk_widget_set_sensitive (dia->butnext, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (dia->butprev, TRUE);
      gtk_widget_set_sensitive (dia->butnext, TRUE);
    }

  gtk_widget_show_all (dia->content);
}

static void
diff_add_video (diff_dialog *dia, const file_node *afn, const file_node *bfn)
{
  GtkWidget *hbox, *hbox2;
  GtkWidget *entry, *label, *headortail;
  gchar count[10];

  dia->afn = afn;
  dia->bfn = bfn;

  /* screenshot */
  dia->container = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (dia->content), dia->container, TRUE, TRUE, 0);

  /* head or tail */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_end (GTK_BOX (dia->content), hbox, FALSE, FALSE, 5);

  headortail = gtk_toggle_button_new_with_label (_ ("From Tail"));
  gtk_box_pack_start (GTK_BOX (hbox), headortail, FALSE, FALSE, 2);
  g_signal_connect (G_OBJECT (headortail), "toggled",
		    G_CALLBACK (diffdia_onheadtail), dia);

  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 2);
  label = gtk_label_new (_ ("Compare Count"));
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 2);
  entry = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox2), entry, FALSE, FALSE, 2);
  g_signal_connect (G_OBJECT (entry), "changed",
		    G_CALLBACK (diffdia_onseekchanged), dia);

  dia->butprev = gtk_button_new_with_label (_ ("Previous"));
  gtk_box_pack_start (GTK_BOX (hbox), dia->butprev, FALSE, FALSE, 2);
  g_signal_connect (G_OBJECT (dia->butprev), "clicked",
		    G_CALLBACK (diffdia_onprev), dia);
  dia->butnext = gtk_button_new_with_label (_ ("Next"));
  gtk_box_pack_start (GTK_BOX (hbox), dia->butnext, FALSE, FALSE, 2);
  g_signal_connect (G_OBJECT (dia->butnext), "clicked",
		    G_CALLBACK (diffdia_onnext), dia);

  g_snprintf (count, sizeof count, "%d", g_ini->compare_count);
  gtk_entry_set_text (GTK_ENTRY (entry), count);
  dia->index = 1;
  diffdia_refresh_video_pic (dia);
}

static void
restree_diff (GtkMenuItem *item, gui_t *gui)
{
  diff_dialog_new (gui, gui->resselfiles[0], gui->resselfiles[1]);
}

static void
diff_dialog_new (gui_t *gui, const file_node *afn, const file_node *bfn)
{
  diff_dialog *diffdia;

  diffdia = g_malloc0 (sizeof (diff_dialog));

  diffdia->gui = gui;

  diffdia->dialog = gtk_dialog_new_with_buttons ("fdupves diff dialog",
						 GTK_WINDOW (gui->widget),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_STOCK_CLOSE,
						 GTK_RESPONSE_CLOSE,
						 NULL);

  diffdia->content = gtk_dialog_get_content_area (GTK_DIALOG (diffdia->dialog));

  g_signal_connect (G_OBJECT (diffdia->dialog), "response",
		    G_CALLBACK (diffdia_onresponse), diffdia);
  g_signal_connect (G_OBJECT (diffdia->dialog), "close",
		    G_CALLBACK (diffdia_onclose), diffdia);

  if (afn->type == FD_IMAGE && bfn->type == FD_IMAGE)
    {
      diff_add_image (diffdia, afn, bfn);
    }
  else if (afn->type == FD_VIDEO && bfn->type == FD_VIDEO)
    {
      diff_add_video (diffdia, afn, bfn);
    }
  else
    {
      gchar *desc;
      GtkWidget *label;

      desc = g_strdup_printf ("file type are not same, can't show diff\n"
			      "file 1: %s\n"
			      "file 2: %s\n",
			      afn->path, bfn->path);
      label = gtk_label_new (desc);
      g_free (desc);
      gtk_box_pack_start (GTK_BOX (diffdia->content), label, TRUE, FALSE, 2);
    }
  gtk_widget_show_all (diffdia->content);

  gtk_dialog_run (GTK_DIALOG (diffdia->dialog));
}

static void
diffdia_onnext (GtkWidget *but, diff_dialog *dialog)
{
  dialog->index += 1;
  diffdia_refresh_video_pic (dialog);
}

static void
diffdia_onprev (GtkWidget *but, diff_dialog *dialog)
{
  dialog->index -= 1;
  diffdia_refresh_video_pic (dialog);
}

static void
diffdia_onclose (GtkWidget *dia, diff_dialog *dialog)
{
  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}

static void
diffdia_onresponse (GtkWidget *dia, gint res, diff_dialog *dialog)
{
  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}

static void
diffdia_onheadtail (GtkToggleButton *but, diff_dialog *dia)
{
  dia->from_tail = gtk_toggle_button_get_active (but);
}

static void
diffdia_onseekchanged (GtkEntry *entry, diff_dialog *dia)
{
  const gchar *text;
  int cnt;

  text = gtk_entry_get_text (entry);
  cnt = atoi (text);
  if (cnt < 1 || cnt == g_ini->compare_count)
    {
      return;
    }

  g_ini->compare_count = cnt;

  dia->index = 1;
  diffdia_refresh_video_pic (dia);
}

static void
gui_find_step_cb (const find_step *step, gui_t *gui)
{
  if (step->doing)
    {
      gdk_threads_enter ();
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (gui->progress),
				 step->doing);
      gdk_threads_leave ();
    }

  if (step->total > 0 && step->now < step->total)
    {
      gdk_threads_enter ();
      gtk_progress_bar_set_fraction
	(GTK_PROGRESS_BAR (gui->progress),
	 (gdouble) step->now / (gdouble) step->total);
      gdk_threads_leave ();
    }

  if (step->found)
    {
      gui->same_list = gui_append_same_slist (gui,
					      gui->same_list,
					      step->afile,
					      step->bfile,
					      step->type
					      );
    }
}

static GSList *
gui_append_same_slist (gui_t *gui, GSList *slist,
		       const gchar *afile, const gchar *bfile,
		       same_type type)
{
  GSList *cur, *fslist;
  same_node *node;
  file_node *fn, *fn2;
  gboolean afind, bfind;
  GtkTreeIter itr[1], itrc[1];
  GtkTreePath *path;

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
	  fn = fslist->data;

	  if (strcmp (fn->path, afile) == 0)
	    {
	      afind = TRUE;
	    }
	  else if (strcmp (fn->path, bfile) == 0)
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
	  fn = file_node_new (node,
			      bfile,
			      type == FD_SAME_IMAGE ?
			      FD_IMAGE : FD_VIDEO);

	  gdk_threads_enter ();
	  if (node->show)
	    {
	      path = gtk_tree_row_reference_get_path (node->treerowref);
	      gtk_tree_model_get_iter (GTK_TREE_MODEL (gui->restreestore), itr, path);
	      gtk_tree_store_append (gui->restreestore, itrc, itr);
	      file_node_to_tree_iter (fn, gui->restreestore, itrc);
	      gtk_tree_path_free (path);
	    }
	  gdk_threads_leave ();

	  return slist;
	}
      else if (bfind)
	{
	  fn = file_node_new (node,
			      afile,
			      type == FD_SAME_IMAGE?
			      FD_IMAGE : FD_VIDEO);

	  gdk_threads_enter ();
	  if (node->show)
	    {
	      path = gtk_tree_row_reference_get_path (node->treerowref);
	      gtk_tree_model_get_iter (GTK_TREE_MODEL (gui->restreestore), itr, path);
	      gtk_tree_store_append (gui->restreestore, itrc, itr);
	      file_node_to_tree_iter (fn, gui->restreestore, itrc);
	      gtk_tree_path_free (path);
	    }
	  gdk_threads_leave ();

	  return slist;
	}
    }

  node = g_malloc0 (sizeof (same_node));
  g_return_val_if_fail (node, slist);

  node->type = type;
  fn = file_node_new (node,
		      afile,
		      type == FD_SAME_IMAGE ?
		      FD_IMAGE : FD_VIDEO);
  fn2 = file_node_new (node,
		       bfile,
		       type == FD_SAME_IMAGE ?
		       FD_IMAGE : FD_VIDEO);

  gdk_threads_enter ();
  gtk_tree_store_append (gui->restreestore, itr, NULL);
  file_node_to_tree_iter (fn, gui->restreestore, itr);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (gui->restreestore), itr);
  node->treerowref = gtk_tree_row_reference_new (GTK_TREE_MODEL (gui->restreestore), path);
  gtk_tree_path_free (path);
  gtk_tree_store_append (gui->restreestore, itrc, itr);
  file_node_to_tree_iter (fn2, gui->restreestore, itrc);
  node->show = TRUE;
  gdk_threads_leave ();

  return g_slist_append (slist, node);
}

void
same_node_free (same_node *node)
{
  if (node->treerowref)
    {
      gtk_tree_row_reference_free (node->treerowref);
      node->treerowref = NULL;
    }
  g_slist_foreach (node->files, (GFunc) file_node_free, NULL);
  g_slist_free (node->files);
  g_free (node);
}

void
same_list_free (GSList *list)
{
  g_slist_foreach (list, (GFunc) same_node_free, NULL);
  g_slist_free (list);
}

static void
restree_filter_changed (GtkEntry *entry, gui_t *gui)
{
  gui_filter_result (gui, gtk_entry_get_text (entry));
}

static void
restree_filter_focusin (GtkEntry *entry, gui_t *gui)
{
  gtk_entry_set_text (GTK_ENTRY (entry), "");
  g_signal_handlers_disconnect_by_func (G_OBJECT (entry),
					G_CALLBACK (restree_filter_focusin), gui);
}

static void
restree_selcombo_changed (GtkComboBox *comtext, gui_t *gui)
{
  gint id;

  id = gtk_combo_box_get_active (comtext);
  switch (id)
    {
    case 0:
      break;

    case 1:
      g_slist_foreach (gui->same_list, (GFunc) restree_sel_small_file, gui);
      break;
    case 2:
      g_slist_foreach (gui->same_list, (GFunc) restree_sel_big_file, gui);
      break;
    case 3:
      g_slist_foreach (gui->same_list, (GFunc) restree_sel_small_image, gui);
      break;
    case 4:
      g_slist_foreach (gui->same_list, (GFunc) restree_sel_big_image, gui);
      break;
    case 5:
      g_slist_foreach (gui->same_list, (GFunc) restree_sel_short_video, gui);
      break;
    case 6:
      g_slist_foreach (gui->same_list, (GFunc) restree_sel_long_video, gui);
      break;
    case 7:
      g_slist_foreach (gui->same_list, (GFunc) restree_sel_others, gui);
      break;

    default:
      break;
    }
}

static void
gui_delsel_cb (GtkWidget *but, gui_t *gui)
{
  if (gui->resselfiles && gui->resselfiles[0])
    {
      restree_delete (NULL, gui);
    }
}

static file_node *
file_node_new (same_node *node, const gchar *path, gint type)
{
  file_node *fn;
#ifdef WIN32
  struct _stat32 buf[1];
#else
  struct stat buf[1];
#endif

  fn = g_malloc0 (sizeof (file_node));
  g_return_val_if_fail (fn, NULL);

  fn->path = g_strdup (path);
  fn->name = g_path_get_basename (path);
  fn->dir = g_path_get_dirname (path);

  if (g_stat (path, buf) == 0)
    {
      fn->size = buf->st_size;
    }

  fn->type = type;

  if (type == FD_IMAGE)
    {
      GdkPixbufFormat *format;

      format = gdk_pixbuf_get_file_info (path,
					 &fn->width, &fn->height);
      if (format)
	{
	  fn->format = gdk_pixbuf_format_get_name (format);
	}

    }
  else if (type == FD_VIDEO)
    {
      video_info *info;

      info = video_get_info (path);
      if (info)
	{
	  fn->width = info->size[0];
	  fn->height = info->size[1];
	  fn->length = info->length;
	  fn->format = g_strdup (info->format);
	  video_info_free (info);
	}
    }

  fn->node = node;
  node->files = g_slist_append (node->files, fn);

  return fn;
}

static void
file_node_free (file_node *fn)
{
  g_free (fn->path);
  g_free (fn->name);
  g_free (fn->dir);
  g_free (fn->format);
  g_free (fn);
}

static void
file_node_free_full (file_node *fn)
{
  same_node *node;

  node = fn->node;

  if (g_cache)
    {
      cache_remove (g_cache, fn->path);
    }

  file_node_free (fn);
  node->files = g_slist_remove (node->files, fn);

  if (g_slist_length (node->files) == 1)
    {
      file_node_free_full (node->files->data);
    }
}

static void
file_node_to_tree_iter (file_node *fn, GtkTreeStore *store, GtkTreeIter *itr)
{
  gchar isizestr[0x100], vlenstr[0x100], *fsizestr;

  g_snprintf (isizestr, sizeof isizestr, "%u:%u",
	      fn->width, fn->height);
  g_snprintf (vlenstr, sizeof vlenstr, "%f",
	      fn->length);
#if GLIB_CHECK_VERSION (2, 30, 0)
  fsizestr = g_format_size (fn->size);
#else
  fsizestr = g_strdup_printf ("%d", fn->size);
#endif

  gtk_tree_store_set (store, itr,
		      0, fn->path,
		      1, isizestr,
		      2, fsizestr,
		      3, vlenstr,
		      4, fn->format,
		      5, fn,
		      -1);
  g_free (fsizestr);
}

static void
restree_select_rowref_id (gui_t *gui, GtkTreeRowReference *ref, gint id)
{
  GtkTreePath *path;

  path = gtk_tree_row_reference_get_path (ref);
  if (id > 0)
    {
      gtk_tree_path_down (path);
      while (-- id)
	{
	  gtk_tree_path_next (path);
	}
    }
  gtk_tree_selection_select_path (gui->resselect, path);
}

static void
restree_unselect_rowref_id (gui_t *gui, GtkTreeRowReference *ref, gint id)
{
  GtkTreePath *path;

  path = gtk_tree_row_reference_get_path (ref);
  if (id > 0)
    {
      gtk_tree_path_down (path);
      while (-- id)
	{
	  gtk_tree_path_next (path);
	}
    }
  gtk_tree_selection_unselect_path (gui->resselect, path);
}

static void
restree_sel_small_file (same_node *node, gui_t *gui)
{
  GSList *slist;
  file_node *fn, *sfn;

  if (!node->show)
    {
      return;
    }

  for (sfn = node->files->data, slist = node->files;
       slist != NULL;
       slist = g_slist_next (slist))
    {
      fn = slist->data;

      fn->selected = FALSE;
      restree_unselect_rowref_id (gui, node->treerowref,
				  g_slist_index (node->files, fn));
      if (fn->size < sfn->size)
	{
	  sfn = fn;
	}
    }

  sfn->selected = TRUE;
  restree_select_rowref_id (gui, node->treerowref,
			    g_slist_index (node->files, sfn));
}

static void
restree_sel_big_file (same_node *node, gui_t *gui)
{
  GSList *slist;
  file_node *fn, *sfn;

  if (!node->show)
    {
      return;
    }

  for (sfn = node->files->data, slist = node->files;
       slist != NULL;
       slist = g_slist_next (slist))
    {
      fn = slist->data;

      fn->selected = FALSE;
      restree_unselect_rowref_id (gui, node->treerowref,
				  g_slist_index (node->files, fn));
      if (fn->size > sfn->size)
	{
	  sfn = fn;
	}
    }

  sfn->selected = TRUE;
  restree_select_rowref_id (gui, node->treerowref,
			    g_slist_index (node->files, sfn));
}

static void
restree_sel_small_image (same_node *node, gui_t *gui)
{
  GSList *slist;
  file_node *fn, *sfn;

  if (!node->show)
    {
      return;
    }

  for (sfn = node->files->data, slist = node->files;
       slist != NULL;
       slist = g_slist_next (slist))
    {
      fn = slist->data;

      fn->selected = FALSE;
      restree_unselect_rowref_id (gui, node->treerowref,
				  g_slist_index (node->files, fn));
      if (fn->width * fn->height < sfn->width * sfn->height)
	{
	  sfn = fn;
	}
    }

  sfn->selected = TRUE;
  restree_select_rowref_id (gui, node->treerowref,
			    g_slist_index (node->files, sfn));
}

static void
restree_sel_big_image (same_node *node, gui_t *gui)
{
  GSList *slist;
  file_node *fn, *sfn;

  if (!node->show)
    {
      return;
    }

  for (sfn = node->files->data, slist = node->files;
       slist != NULL;
       slist = g_slist_next (slist))
    {
      fn = slist->data;

      fn->selected = FALSE;
      restree_unselect_rowref_id (gui, node->treerowref,
				  g_slist_index (node->files, fn));
      if (fn->width * fn->height > sfn->width * sfn->height)
	{
	  sfn = fn;
	}
    }

  sfn->selected = TRUE;
  restree_select_rowref_id (gui, node->treerowref,
			    g_slist_index (node->files, sfn));
}

static void
restree_sel_short_video (same_node *node, gui_t *gui)
{
  GSList *slist;
  file_node *fn, *sfn;

  if (!node->show)
    {
      return;
    }

  for (sfn = node->files->data, slist = node->files;
       slist != NULL;
       slist = g_slist_next (slist))
    {
      fn = slist->data;

      fn->selected = FALSE;
      restree_unselect_rowref_id (gui, node->treerowref,
				  g_slist_index (node->files, fn));
      if (fn->length < sfn->length)
	{
	  sfn = fn;
	}
    }

  sfn->selected = TRUE;
  restree_select_rowref_id (gui, node->treerowref,
			    g_slist_index (node->files, sfn));
}

static void
restree_sel_long_video (same_node *node, gui_t *gui)
{
  GSList *slist;
  file_node *fn, *sfn;

  if (!node->show)
    {
      return;
    }

  for (sfn = node->files->data, slist = node->files;
       slist != NULL;
       slist = g_slist_next (slist))
    {
      fn = slist->data;

      fn->selected = FALSE;
      restree_unselect_rowref_id (gui, node->treerowref,
				  g_slist_index (node->files, fn));
      if (fn->length > sfn->length)
	{
	  sfn = fn;
	}
    }

  sfn->selected = TRUE;
  restree_select_rowref_id (gui, node->treerowref,
			    g_slist_index (node->files, sfn));
}

static void
restree_sel_others (same_node *node, gui_t *gui)
{
  GSList *slist;
  file_node *fn;

  if (!node->show)
    {
      return;
    }

  for (slist = node->files;
       slist != NULL;
       slist = g_slist_next (slist))
    {
      fn = slist->data;

      if (fn->selected)
	{
	  fn->selected = FALSE;
	  restree_unselect_rowref_id (gui, node->treerowref,
				      g_slist_index (node->files, fn));
	}
      else
	{
	  fn->selected = TRUE;
	  restree_select_rowref_id (gui, node->treerowref,
				    g_slist_index (node->files, fn));
	}
    }
}
