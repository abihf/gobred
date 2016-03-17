/* 
 * Copyright 2016 Abi Hafshin <abi@hafs.in>
 * 
 * This file is part of libgobred 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <JavaScriptCore/JavaScript.h>
#include "gobred.h"
#include "gobred-private.h"
#include "gobred-js-utils.h"

#define MAX_EVENT_NAME 64

typedef struct  {
  JSContextRef context;
  JSObjectRef cb_func;
  GobredModuleData *module_data;
  GMainContext *thread_context;
} EventListener;


static void
event_listener_destroy (EventListener *el){
  gobred_module_data_unref (el->module_data);
  g_slice_free (EventListener, el);
}

static GTree *events;

static gint
_event_tree_compare (gconstpointer a,
                     gconstpointer b,
                     gpointer user_data)
{
  return g_ascii_strcasecmp (a, b);
}


static void
_event_tree_free_value (GList *ptr)
{
  g_list_free_full (ptr, (GDestroyNotify)event_listener_destroy);
}

void
gobred_event_prepare (const GobredModuleDefinitionBase *modules[])
{
  g_print("event prepare\n");
  events = g_tree_new_full (_event_tree_compare,
                            NULL,
                            g_free,
                            (GDestroyNotify)_event_tree_free_value);

  const GobredModuleDefinitionBase *module;
  gchar *event_name;

  for (int i=0; modules[i]; i++) {
    module = modules[i];
    switch (modules[i]->version) {
    case GOBRED_MODULE_DEFINITION_VERSION_0:
        {
          GobredEventDefinitionV0 *e = ((GobredModuleDefinitionV0*)modules[i])->events;
          for (; e && e->name; e++) {
            event_name = g_strdup_printf ("%s.%s", module->name, e->name);
            g_tree_replace (events, event_name, g_list_alloc());
          }
        }
        break;
    }

  }
}


void
gobred_event_clean ()
{
  g_tree_unref (events);
}

JSValueRef
gobred_event_handle_add_listener (JSContextRef ctx,
			    JSObjectRef this_func,
			    JSObjectRef this_object,
			    size_t argc,
			    const JSValueRef args[],
			    JSValueRef* exception)
{
  GobredModuleData *module_data = gobred_module_data_from_js(this_object);
  gboolean success = false;

  if (module_data) {
    gchar *name = js_value_get_string (ctx, args[0]);
    gchar *full_name = g_strdup_printf ("%s.%s",
                                        module_data->definition->name,
                                        name);
    g_free (name);
    GList *list = g_tree_lookup (events, full_name);

    if (list != NULL) {
      EventListener *el = g_slice_new0 (EventListener);
      el->context = JSContextGetGlobalContext(ctx);
      el->cb_func = JSValueToObject (ctx, args[1], NULL);
      el->module_data = gobred_module_data_ref (module_data);
      el->thread_context = g_main_context_get_thread_default();
      list = g_list_append (list, el);
      JSValueProtect (ctx, el->cb_func);
      success = true;
    }
    g_free (full_name);
  }
  return JSValueMakeBoolean (ctx, success);
}

JSValueRef
gobred_event_handle_remove_listener (JSContextRef ctx,
			    JSObjectRef this_func,
			    JSObjectRef this_object,
			    size_t argc,
			    const JSValueRef args[],
			    JSValueRef* exception)
{
  GobredModuleData *module_data = gobred_module_data_from_js(this_object);
  gboolean success = false;



  if (module_data) {
    gchar *name = js_value_get_string (ctx, args[0]);
    gchar *full_name = g_strdup_printf ("%s.%s",
                                        module_data->definition->name,
                                        name);
    GList *list = g_tree_lookup (events, full_name);

    if (list != NULL) {
      JSObjectRef js_cb = JSValueToObject (ctx, args[1], NULL);
      GList *item;

      for (item = g_list_first(list); !success && item; item = item->next) {
        EventListener *el = (EventListener *) item->data;
        if (el->context == ctx && el->cb_func == js_cb) {
          event_listener_destroy (el);
          list = g_list_remove_link (list, item);
          success = true;
        }
      }
    }
    if (success) { // list changed
      g_tree_replace (events, full_name, list);
    }

    g_free (name);
    g_free (full_name);

  }
  return JSValueMakeBoolean (ctx, success);
}

/*
typedef struct  {
  JSContextRef ctx;
  JSObjectRef cb_func;
  GobredValue *params;
} EventEmitData;

static gboolean
_emit_source_cb (gpointer pdata)
{
  JSValueRef js_params[1];
  EventEmitData *data = (EventEmitData *)pdata;
  if (data->params) {
    g_print("sampai sini masih ada\n");
    js_params[0] = gobred_value_to_js (data->params, data->ctx);
    g_print("ini ga dipanggil\n");
    JSObjectCallAsFunction (data->ctx, data->cb_func, NULL, 1, js_params, NULL);
    JSValueUnprotect (data->ctx, js_params[0]);
    gobred_value_unref (data->params);
  } else {
    JSObjectCallAsFunction (data->ctx, data->cb_func, NULL, 0, NULL, NULL);
  }
  g_slice_free (EventEmitData, data);
  return G_SOURCE_REMOVE;
}
*/

void
gobred_event_emit (const gchar *event_name,
                   GobredValue *param)
{
  GList *list = g_tree_lookup (events, event_name);

  if (param) {
    param = gobred_value_ref_sink (param);
  }
  if (list) {
    g_print("list ketemu\n");
    GList *item;
    gboolean changed = false;
    for (item = list->next; item && item->data;) {
      GList *tmp_next = item->next;
      EventListener *el = (EventListener *) item->data;
      if (el->module_data->active) {
        if (param) {
          JSValueRef js_params[1];
          js_params[0] = gobred_value_to_js (param, el->context);
          JSObjectCallAsFunction (el->context, el->cb_func, NULL, 1, js_params, NULL);
          JSValueUnprotect (el->context, js_params[0]);
        } else {
          JSObjectCallAsFunction (el->context, el->cb_func, NULL, 0, NULL, NULL);
        }
      } else {
        event_listener_destroy (el);
        list = g_list_remove_link (list, item);
        changed = true;
      }
      item = tmp_next;
    }
    if (changed) {
      g_tree_insert (events, (gchar *)event_name, list);
    }
  }


  if (param) {
    gobred_value_unref (param);
    g_print("data ada\n");

  }
}

