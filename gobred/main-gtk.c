/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2015 Abi Hafshin <abi@hafs.in>
 *
 * gobred is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gobred is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
//#include "gobred.h"

static void
initialize_web_extensions (WebKitWebContext *context)
{
  g_debug ("web extension initialize");
  gchar *dir = PACKAGE_LIB_DIR "/webkit";
  g_print("extension dir %s\n", dir);
  webkit_web_context_set_web_extensions_directory (context, dir);
}

static void
title_changed (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
  const gchar *title = webkit_web_view_get_title (WEBKIT_WEB_VIEW(gobject));
  gchar *window_title = g_strdup_printf ("%s - Gobred", title);
  gtk_window_set_title (GTK_WINDOW (user_data), window_title);
  g_free (window_title);
}

int
main (int argc, char **argv)
{
  gchar *url = "file:///home/abie/Work/projects/dsti/e-akses/gobred/index.html";
  gtk_init (&argc, &argv);

  if (argc > 1)
    url = argv[1];

  WebKitWebContext *context = webkit_web_context_get_default ();
  g_signal_connect(context, "initialize-web-extensions",
		   G_CALLBACK(initialize_web_extensions), NULL);

  WebKitSettings *settings = webkit_settings_new_with_settings (
      "enable-developer-extras", TRUE, NULL);
  GtkWidget *view = webkit_web_view_new_with_settings (settings);
  g_object_unref (settings);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect(window, "destroy", gtk_main_quit, NULL);
  gtk_container_add (GTK_CONTAINER (window), view);

  gtk_window_set_default_size (GTK_WINDOW (window), 1200, 650);
  g_signal_connect(G_OBJECT(view), "notify::title", G_CALLBACK(title_changed),
		   window);

  gtk_widget_show_all (window);
  webkit_web_view_load_uri (WEBKIT_WEB_VIEW(view), url);

  WebKitWebInspector *inspector = webkit_web_view_get_inspector (
      WEBKIT_WEB_VIEW(view));
  webkit_web_inspector_show (inspector);
  //g_object_unref (inspector);

  gtk_main ();

  return 0;
}
