/*
 * main with GUI
 *
 * Copyright (c) 2010, 2011 lxd <edl.eppc@gmail.com>
 * 
 * This file is part of File Synchronization System(fss).
 *
 * fss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, 
 * (at your option) any later version.
 *
 * fss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with fss.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <pthread.h>
#include "files.h"
#include "sock.h"
#include "params.h"
#include "protocol.h"
#include "wrap-inotify.h"
#include "fss.h"

static pthread_t tid;
static char server[VALUE_LEN];
static char path[VALUE_LEN];

static void *thread_main(void *arg);
static void repaint_window_pref();
static void destroy(GtkWidget *widget, gpointer data);
static void trayPref(GtkMenuItem *item, gpointer user_data);
static void trayExit(GtkMenuItem *item, gpointer user_data);
static void trayIconActivated(GObject *trayIcon, gpointer data);
static void trayIconPopup(GtkStatusIcon *status_icon, guint button,
			  guint32 activate_time, gpointer popUpMenu);
static gboolean delete_event (GtkWidget*, GdkEvent*, gpointer);
static void button_path_clicked(GObject*, gpointer);
static void button_ok_select_clicked(GObject*, GtkFileSelection*);
static void button_ok_clicked(GObject*, gpointer);

int main(int argc, char *argv[])
{
  setbuf(stdout, NULL);
  gtk_init (&argc, &argv);

  pthread_create(&tid, NULL, thread_main, NULL);

  GtkStatusIcon *trayIcon  = gtk_status_icon_new_from_file ("logo.png");
  GtkWidget *menu, *menuItemPref, *menuItemExit;
  
  menu = gtk_menu_new();
  menuItemPref = gtk_menu_item_new_with_label ("Preferences");
  menuItemExit = gtk_menu_item_new_with_label ("Exit");

  g_signal_connect (G_OBJECT (menuItemPref), "activate",
		    G_CALLBACK (trayPref), NULL);
  g_signal_connect (G_OBJECT (menuItemExit), "activate",
		    G_CALLBACK (trayExit), NULL);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuItemPref);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuItemExit);
  gtk_widget_show_all (menu);
  
  gtk_status_icon_set_tooltip (trayIcon, "File Synchronization System");
  
  g_signal_connect(GTK_STATUS_ICON (trayIcon), "activate",
		   GTK_SIGNAL_FUNC (trayIconActivated), NULL);
  g_signal_connect(GTK_STATUS_ICON (trayIcon), "popup-menu",
		   GTK_SIGNAL_FUNC (trayIconPopup), menu);
  
  gtk_status_icon_set_visible(trayIcon, TRUE);

  gtk_main ();
  return 0;
}

void *thread_main(void *arg)
{
  int monitor_fd;
  int sockfd;

  setbuf(stdout, NULL);
  memset(path, 0, VALUE_LEN);
  memset(server, 0, VALUE_LEN);


  if (get_param_value("Path", path)) {
    perror("@main(): get_param_value Path fails\n");
    pthread_exit(NULL);
  }
  if (!strlen(path)) {
    perror("Path in fss.conf cannot be empty");
    pthread_exit(NULL);
  }
  if (get_param_value("Server", server)) {
    perror("@main(): get_param_value Server fails\n");
    pthread_exit(NULL);
  }
  if (!strlen(server)) {
    perror("Server in fss.conf cannot be empty");
    pthread_exit(NULL);
  }

  // set monitored directory for file.c
  if (set_rootpath(path)) {
    fprintf(stderr, "@main(): set_rootpath() failed\n");
    pthread_exit(NULL);
  }

  if (update_files()) {
    fprintf(stderr,  "@main(): update_files() failed\n");
    pthread_exit(NULL);
  }
  
  if (monitors_init(path,
		    IN_MODIFY|IN_CREATE|IN_DELETE|
		    IN_MOVED_FROM|IN_MOVED_TO, &monitor_fd)) {
    fprintf(stderr, "@main(): monitors_init() fails\n");
    pthread_exit(NULL);
  }
  if (fss_connect(server, &sockfd)) {
    fprintf(stderr, "@main(): fss_connect() fails\n");
    pthread_exit(NULL);
  }

  if (client_polling(monitor_fd, sockfd)) {
    fprintf(stderr, "@main(): client_polling() failed\n");
    pthread_exit(NULL);
  }
  
  return ((void *)0);

}


GdkPixbuf *create_pixbuf(const gchar * filename)
{
   GdkPixbuf *pixbuf;
   GError *error = NULL;
   pixbuf = gdk_pixbuf_new_from_file(filename, &error);
   if(!pixbuf) {
      fprintf(stderr, "%s\n", error->message);
      g_error_free(error);
   }

   return pixbuf;
}


static void repaint_window_pref()
{
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (window),
			"File Sync System Configuration");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, -1);
  gtk_window_set_icon(GTK_WINDOW(window), create_pixbuf("logo.png"));
  gtk_container_set_border_width(GTK_CONTAINER(window), 20);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  g_signal_connect(G_OBJECT(window), "delete_event",
		   G_CALLBACK(delete_event), NULL);
  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (destroy), window);
  
  GtkWidget *table = gtk_table_new(3, 4, FALSE);
  gtk_container_add(GTK_CONTAINER(window), table);

  GtkWidget *label_server = gtk_label_new("Server: ");
  gtk_table_attach(GTK_TABLE(table), label_server, 0, 1, 0, 1, 0, 0, 5, 5);

  GtkWidget *entry_server = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry_server), server);
  gtk_widget_set_size_request(entry_server, 300, -1);
  gtk_table_attach(GTK_TABLE(table), entry_server, 1, 4, 0, 1, 0, 0, 5, 5);

  GtkWidget *entry_path = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry_path), path);
  gtk_widget_set_size_request(entry_path, 300, -1);
  gtk_table_attach(GTK_TABLE(table), entry_path, 1, 4, 1, 2, 0, 0, 5, 5);
  
  GtkWidget *button_path = gtk_button_new_with_label("Path: ");
  gtk_table_attach(GTK_TABLE(table), button_path, 0, 1, 1, 2, 0, 0, 5, 5);
  g_signal_connect(GTK_OBJECT(button_path), "clicked",
		   G_CALLBACK(button_path_clicked), entry_path);

  
  GtkWidget *button_ok = gtk_button_new_with_label("OK");
  gtk_widget_set_size_request(button_ok, 100, -1);
  gtk_table_attach(GTK_TABLE(table), button_ok, 2, 3, 2, 3, 0, 0, 5, 5);
  
  // associate entry_path's content and entry_server's content to button_ok
  g_object_set_data(G_OBJECT(button_ok), "ENTRY_PATH", entry_path);
  g_object_set_data(G_OBJECT(button_ok), "ENTRY_SERVER", entry_server);

  g_signal_connect(G_OBJECT(button_ok), "clicked",
		   G_CALLBACK(button_ok_clicked), window);
  

  GtkWidget *button_cancel = gtk_button_new_with_label("Cancel");
  gtk_widget_set_size_request(button_cancel, 100, -1);
  gtk_table_attach(GTK_TABLE(table), button_cancel, 3, 4, 2, 3, 0, 0, 5, 5);
  g_signal_connect(G_OBJECT(button_cancel), "clicked",
		   G_CALLBACK(destroy), window);

  gtk_widget_show_all(window);

}

static void destroy(GtkWidget *widget, gpointer data)
{
  printf("in destroy\n");
  gtk_widget_destroy(GTK_WIDGET(data));
}

static void trayPref(GtkMenuItem *item, gpointer data) 
{
  pthread_cancel(tid);
  repaint_window_pref();
}

static void trayExit(GtkMenuItem *item, gpointer data) 
{
  
  gtk_main_quit();
}


static void trayIconActivated(GObject *trayIcon, gpointer data)
{
  repaint_window_pref();
}

static void trayIconPopup(GtkStatusIcon *status_icon,
			  guint button,
			  guint32 activate_time,
			  gpointer popUpMenu)
{
  gtk_menu_popup(GTK_MENU(popUpMenu), NULL, NULL,
		 gtk_status_icon_position_menu,
		 status_icon,
		 button,
		 activate_time);
}

static void button_path_clicked(GObject *buttton, gpointer data)
{
  GtkWidget *dialog = gtk_file_selection_new("Select a Directory");
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  
  g_signal_connect(G_OBJECT(dialog), "destroy",
		   G_CALLBACK(gtk_widget_destroy), &dialog);

  g_object_set_data(G_OBJECT(dialog), "ENTRY_PATH", data);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(dialog)->ok_button),
		   "clicked",
		   G_CALLBACK(button_ok_select_clicked),
		   GTK_FILE_SELECTION(dialog));
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(dialog)->cancel_button),
  		   "clicked",
  		   G_CALLBACK(button_ok_select_clicked),
		   GTK_FILE_SELECTION(dialog));
  
  gtk_widget_show(dialog);

}

static void button_ok_select_clicked(GObject *button, GtkFileSelection *fs)
{
  const gchar *dirname = gtk_file_selection_get_filename(fs);
  gpointer entry_path_ptr = g_object_get_data(G_OBJECT(fs), "ENTRY_PATH");

  if (g_file_test(dirname, G_FILE_TEST_IS_DIR)) {
    
    gtk_entry_set_text(GTK_ENTRY(entry_path_ptr), dirname);
    gtk_widget_destroy(GTK_WIDGET(fs));
    
  } else {

    GtkWidget *dialog=gtk_message_dialog_new(NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT|
					     GTK_DIALOG_MODAL,
					     GTK_MESSAGE_ERROR,
					     GTK_BUTTONS_OK,
					     "It should be a directory");


    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

  }

}

static void button_ok_clicked(GObject *button, gpointer data)
{
  gchar *msg = NULL;
  gpointer entry_path = g_object_get_data(G_OBJECT(button), "ENTRY_PATH");
  gpointer entry_server = g_object_get_data(G_OBJECT(button),
					    "ENTRY_SERVER");
  const gchar *entry_path_text = gtk_entry_get_text(GTK_ENTRY(entry_path));
  const gchar *entry_server_text =
    gtk_entry_get_text(GTK_ENTRY(entry_server));

  if (strlen(entry_path_text) == 0 && strlen(entry_server_text) == 0)
    msg = "\"Server\" and \"Path\" cannot be left blank !";
  else if (strlen(entry_path_text) == 0)
    msg = "\"Path\" cannot be left blank !";
  else if (strlen(entry_server_text) == 0)
    msg = "\"Server\" cannot be left blank !";
  else if (!g_file_test(entry_path_text, G_FILE_TEST_IS_DIR))
    msg = "\"Path\" is invalid !";

  if (msg) {
    GtkWidget *dialog=gtk_message_dialog_new(NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT|
					     GTK_DIALOG_MODAL,
					     GTK_MESSAGE_ERROR,
					     GTK_BUTTONS_OK,
					     msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
  gtk_widget_destroy(GTK_WIDGET(data));
  
}


static gboolean delete_event (GtkWidget *window,
			      GdkEvent *event,
			      gpointer data)
{
  return FALSE;
}
