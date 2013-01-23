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
#include "find.h"
#include "gui.h"

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <string.h>

typedef struct
{
  GtkWidget *widget;
  GtkWidget *mainvbox;

  GtkListStore *dirlist;

  GPtrArray *images;
  GPtrArray *videos;

  GtkListStore *loglist;

  GtkTreeStore *restree;
  GtkTreeSelection *resselect;
  gchar **resselfiles;
} gui_t;

static void toolbar_new (gui_t *);
static void mainframe_new (gui_t *);
static void statusbar_new (gui_t *);

static void gui_add_cb (GtkWidget *, gui_t *);
static void gui_find_cb (GtkWidget *, gui_t *);
static void gui_pref_cb (GtkWidget *, gui_t *);
static void gui_help_cb (GtkWidget *, gui_t *);

static gui_t gui[1];
static void gui_destroy (gui_t *);
static void gui_destroy_cb (GtkWidget *, GdkEvent *, gui_t *);
static void gui_add_dir (gui_t *, const gchar *);
static void gui_add_same_node (same_node *node, gui_t *);

static gboolean dir_find_item (GtkTreeModel *,
			       GtkTreePath *,
			       GtkTreeIter *,
			       gui_t *);
static void gui_list_dir (gui_t *, const gchar *);
static void gui_list_file (gui_t *, const gchar *);
static void gui_list_link (gui_t *, const gchar *);

static void dirlist_onactivated (GtkTreeView *,
				 GtkTreePath *,
				 GtkTreeViewColumn *,
				 gui_t *);
static gboolean restree_onbutpress (GtkWidget *,
				    GdkEventButton *,
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

gboolean
gui_init (int argc, char *argv[])
{
  int i;

  if (ini_new_with_file (FD_USR_CONF_FILE) == FALSE)
    {
      ini_new ();
    }

  gui->widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (gui->widget), "fdupves 0.0.1");
  gtk_window_set_default_size (GTK_WINDOW (gui->widget), 800, 600);

  g_signal_connect (G_OBJECT (gui->widget), "destroy-event", G_CALLBACK (gui_destroy_cb), gui);
  g_signal_connect (G_OBJECT (gui->widget), "delete-event", G_CALLBACK (gui_destroy_cb), gui);

  gui->mainvbox = gtk_vbox_new (FALSE, FALSE);
  gtk_container_add (GTK_CONTAINER (gui->widget), gui->mainvbox);

  toolbar_new (gui);
  mainframe_new (gui);
  statusbar_new (gui);

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
  but = gtk_tool_button_new (img, _ ("Add"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (but), _ ("Add Source Files"));
  g_signal_connect (G_OBJECT (but), "clicked", G_CALLBACK (gui_add_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), but, -1);

  img = fd_toolbar_icon_new ("find.png");
  but = gtk_tool_button_new (img, _ ("Find"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (but), _ ("Find Duplicate Files"));
  g_signal_connect (G_OBJECT (but), "clicked", G_CALLBACK (gui_find_cb), gui);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), but, -1);

  img = fd_toolbar_icon_new ("pref.png");
  but = gtk_tool_button_new (img, _ ("Preference"));
  gtk_widget_set_tooltip_text (GTK_WIDGET (but), _ ("Preference"));
  g_signal_connect (G_OBJECT (but), "clicked", G_CALLBACK (gui_pref_cb), gui);
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
  gui_t *gui;

  gui = (gui_t *) user_data;

  gtk_list_store_append (gui->loglist, itr);
  gtk_list_store_set (gui->loglist, itr, 0, message, -1);
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
mainframe_new (gui_t *gui)
{
  GtkWidget *hpaned, *vpaned, *vbox, *win, *tree;
  GtkWidget *typebox, *typeibut, *typevbut;
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
  gui->dirlist = gtk_list_store_new (1, G_TYPE_STRING);
  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gui->dirlist));
  gtk_container_add (GTK_CONTAINER (win), tree);
  gtk_widget_set_size_request (tree, 300, 200);
  gtk_box_pack_start (GTK_BOX (vbox), win, TRUE, TRUE, 2);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    ("Path",
     renderer, "text", 0,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  g_signal_connect (G_OBJECT (tree), "row-activated", G_CALLBACK (dirlist_onactivated), gui);

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

  /* log win */
  win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
				       GTK_SHADOW_IN);
  gui->loglist = gtk_list_store_new (1, G_TYPE_STRING);
  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gui->loglist));
  gtk_container_add (GTK_CONTAINER (win), tree);
  gtk_paned_add2 (GTK_PANED (vpaned), win);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    ("Message",
     renderer, "text", 0,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  g_log_set_handler (NULL, G_LOG_LEVEL_MASK,
		     gui_log, gui);

  /* result tree */
  win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
				       GTK_SHADOW_IN);
  gui->restree = gtk_tree_store_new (1, G_TYPE_STRING);
  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gui->restree));
  gtk_container_add (GTK_CONTAINER (win), tree);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    ("Result",
     renderer, "text", 0,
     NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_widget_add_events (GTK_WIDGET (tree), GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT (tree), "button-press-event",
		    G_CALLBACK (restree_onbutpress), gui);
  gui->resselect = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (gui->resselect, GTK_SELECTION_MULTIPLE);
  g_signal_connect (G_OBJECT (gui->resselect), "changed",
		    G_CALLBACK (restreesel_onchanged), gui);
  gtk_paned_add2 (GTK_PANED (hpaned), win);
}

static void
statusbar_new (gui_t *gui)
{
  GtkWidget *status;

  status = gtk_statusbar_new ();
  gtk_box_pack_end (GTK_BOX (gui->mainvbox), status, FALSE, FALSE, 2);
}

static void
gui_add_dir (gui_t *gui, const char *path)
{
  GtkTreeIter itr[1];

  gtk_list_store_append (gui->dirlist, itr);
  gtk_list_store_set (gui->dirlist, itr, 0, path, -1);
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
  GSList *listi, *listv;
  gui->images = g_ptr_array_new_with_free_func (g_free);
  gui->videos = g_ptr_array_new_with_free_func (g_free);
  gtk_tree_store_clear (gui->restree);

  gtk_tree_model_foreach (GTK_TREE_MODEL (gui->dirlist),
			  (GtkTreeModelForeachFunc) dir_find_item,
			  gui);

  if (g_ini->proc_image)
    {
      g_message ("find %d images to process", gui->images->len);
      listi = find_images (gui->images);
      g_slist_foreach (listi, (GFunc) gui_add_same_node, gui);
      same_list_free (listi);
    }

  if (g_ini->proc_video)
    {
      g_message ("find %d videos to process", gui->videos->len);
      listv = find_videos (gui->videos);
      g_slist_foreach (listv, (GFunc) gui_add_same_node, gui);
      same_list_free (listv);
    }

  g_ptr_array_free (gui->images, TRUE);
  g_ptr_array_free (gui->videos, TRUE);
}

static void
gui_pref_cb (GtkWidget *wid, gui_t *gui)
{
}

static void
gui_help_cb (GtkWidget *wid, gui_t *gui)
{
}

static void
gui_add_same_node (same_node *node, gui_t *gui)
{
  GtkTreeIter itr[1], itrc[1];
  GSList *cur;

  /* add first file as parent */
  cur = node->files;
  gtk_tree_store_append (gui->restree, itr, NULL);
  gtk_tree_store_set (gui->restree, itr, 0, cur->data, -1);

  /* add children files */
  while ((cur = g_slist_next (cur)) != NULL)
    {
      gtk_tree_store_append (gui->restree, itrc, itr);
      gtk_tree_store_set (gui->restree, itrc, 0, cur->data, -1);
    }
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
	  g_error ("Can't open dir: %s: %s", dir, err->message);
	  g_error_free (err);
	}

      while ((cur = g_dir_read_name (gdir)) != NULL)
	{
	  if (strcmp (cur, ".") == 0
	      || strcmp (cur, "..") == 0)
	    {
	      continue;
	    }

	  curpath = g_strdup_printf ("%s/%s", dir, cur);
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
  size_t i, plen, slen;
  gchar *p, *s;

  plen = strlen (path);

  if (g_ini->proc_image)
    {
      for (i = 0; g_ini->image_suffix[i][0]; ++ i)
	{
	  s = g_ini->image_suffix[i];
	  slen = strlen (s);
	  p = (gchar *) path + plen - slen;
	  if (g_ascii_strcasecmp (p, s) == 0)
	    {
	      p = g_strdup (path);
	      g_ptr_array_add (gui->images, p);
	      break;
	    }
	}
    }

  if (g_ini->proc_video)
    {
      for (i = 0; g_ini->video_suffix[i][0]; ++ i)
	{
	  s = g_ini->video_suffix[i];
	  slen = strlen (s);
	  p = (gchar *) path + plen - slen;
	  if (g_ascii_strcasecmp (p, s) == 0)
	    {
	      p = g_strdup (path);
	      g_ptr_array_add (gui->videos, p);
	      break;
	    }
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
				_ ("Are you sure you want to quit fdupves? ")
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
      (GTK_TREE_MODEL (gui->dirlist),
       itr,
       path))
    {
      gtk_list_store_remove (gui->dirlist, itr);
    }
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
      g_strfreev (gui->resselfiles);
      gui->resselfiles = NULL;
    }

  list = gtk_tree_selection_get_selected_rows (sel, NULL);
  cnt = g_list_length (list);
  if (cnt < 1)
    {
      return;
    }

  gui->resselfiles = g_new0 (gchar *, cnt + 1);

  for (i = 0, cur = list; cur; ++ i, cur = g_list_next (cur))
    {
      gtk_tree_model_get_iter (GTK_TREE_MODEL (gui->restree),
			       itr,
			       cur->data);
      gtk_tree_model_get (GTK_TREE_MODEL (gui->restree), itr,
			  0, gui->resselfiles + i,
			  -1);
      gtk_tree_path_free (cur->data);
    }
  g_list_free (list);
}

static void
restree_open (GtkMenuItem *item, gui_t *gui)
{
}
static void
restree_opendir (GtkMenuItem *item, gui_t *gui)
{
}

static void
gui_remove_not_exists (gui_t *gui, GtkTreeIter *itr)
{
  gsize cnt;
  gchar *file;
  gboolean has;
  GtkTreeIter child[1];

  gtk_tree_model_iter_children (GTK_TREE_MODEL (gui->restree), child, itr);
  while (gtk_tree_store_iter_is_valid (gui->restree, child))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (gui->restree), child, 0, &file, -1);

      if (g_file_test (file, G_FILE_TEST_EXISTS) == FALSE)
	{
	  gtk_tree_store_remove (gui->restree, child);
	}
      else
	{
	  gtk_tree_model_iter_next (GTK_TREE_MODEL (gui->restree), child);
	}

      g_free (file);
    }
  gtk_tree_model_get (GTK_TREE_MODEL (gui->restree), itr, 0, &file, -1);

  cnt = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (gui->restree), itr);
  if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
      has = TRUE;
      ++ cnt;
    }
  else
    {
      has = FALSE;
    }
  g_free (file);

  if (cnt < 2) /* no same files in this branch */
    {
      gtk_tree_store_remove (gui->restree, itr);
      return;
    }

  if (!has) /* parent is exists */
    {
      gtk_tree_model_iter_children (GTK_TREE_MODEL (gui->restree), child, itr);
      gtk_tree_model_get (GTK_TREE_MODEL (gui->restree), child, 0, &file, -1);
      gtk_tree_store_set (gui->restree, itr, 0, file, -1);
      g_free (file);
      gtk_tree_store_remove (gui->restree, child);
    }
}

static void
restree_delete (GtkMenuItem *item, gui_t *gui)
{
  GtkWidget *dia;
  gint i, ret;
  GtkTreeIter itr[1];

  dia = gtk_message_dialog_new (GTK_WINDOW (gui->widget),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_YES_NO,
				_ ("Are you sure you want to delete these files? ")
				);

  ret = gtk_dialog_run (GTK_DIALOG (dia));
  gtk_widget_destroy (dia);

  if (ret == GTK_RESPONSE_YES)
    {
      for (i = 0; gui->resselfiles[i]; ++ i)
	{
	  g_remove (gui->resselfiles[0]);
	}
    }

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (gui->restree), itr);
  while (gtk_tree_store_iter_is_valid (gui->restree, itr))
    {
      gui_remove_not_exists (gui, itr);
      if (gtk_tree_store_iter_is_valid (gui->restree, itr))
	{
	  gtk_tree_model_iter_next (GTK_TREE_MODEL (gui->restree), itr);
	}
    }
}
static void
restree_diff (GtkMenuItem *item, gui_t *gui)
{
}
