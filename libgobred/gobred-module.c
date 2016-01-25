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
#include <gio/gio.h>
#include <JavaScriptCore/JavaScript.h>
#include "config.h"
#include "gobred.h"
#include "gobred-private.h"

//extern GobredModuleDefinition module_gobred_device;
//extern GobredModuleDefinition module_gobred_fingerprint;
//extern GobredModuleDefinition module_gobred_smartcard;
enum {
  GOBRED_ERROR_MODULE_DIR_NOT_FOUND = 0x00010001,
  GOBRED_ERROR_MODULE_DIR_NOT_DIR = 0x00000102,
};

static gboolean initialized = FALSE;
static GList *modules = NULL;

static const GobredModuleDefinition **module_definitions;

typedef struct  {
  const GobredModuleDefinition *definition;
  GModule *gmodule;
} GobredModule;

static void
gobred_module_load_all ()
{
  GFile *dir, *file = NULL;
  GFileEnumerator *enumerator;
  GFileInfo *info = NULL;
  GError *error = NULL;
  GobredModule *module;

  GModule *gmodule;

  const gchar *module_path = g_getenv("GOBRED_MODULE_DIR");
  if (module_path == NULL) module_path = GOBRED_MODULE_DIR;
  dir = g_file_new_for_path(module_path);
  enumerator = g_file_enumerate_children(dir,
                                         G_FILE_ATTRIBUTE_STANDARD_NAME,
                                         G_FILE_QUERY_INFO_NONE,
                                         NULL,
                                         &error);
  if (error != NULL) {
    if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
      gobred_critical (GOBRED_ERROR_MODULE_DIR_NOT_FOUND);
    }
    else if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_DIRECTORY)) {
      gobred_critical (GOBRED_ERROR_MODULE_DIR_NOT_DIR);
    } else {
      g_warning("unknown error %s:%d", g_quark_to_string (error->domain), error->code);
    }
    g_error_free(error);
    return;
  }

  modules = g_list_alloc();
  int module_number = 0;
  while (g_file_enumerator_iterate (enumerator, NULL, &file, NULL, &error)) {
    if (file == NULL) {
      g_print("KOK NULL?\n");
      break;
    }
    gchar *file_name = g_file_get_path(file);
    gmodule = g_module_open (file_name, G_MODULE_BIND_LAZY);
    if (gmodule != NULL) {
      //GobredModuleRegisterFunc register_func;
      gpointer symbol = NULL;
      if (g_module_symbol(gmodule, "gobred_module_register", &symbol)) {
        GobredModuleRegisterFunc register_func = (GobredModuleRegisterFunc)symbol;
        const GobredModuleDefinition *definition = register_func();
        module = g_slice_new (GobredModule);
        module->definition = definition;
        module->gmodule = gmodule;
        modules = g_list_append (modules, module);
        module_number++;
      } else {
        g_module_close(gmodule);
      }
    } else {
      //gobred_warning("[module] Unable to open module %s");
    }
    g_free(file_name);
    file = NULL;
    info = NULL;
  }
  g_object_unref(dir);


  module_definitions = g_new(const GobredModuleDefinition*, module_number+1);
  int i = 0;
  for (GList *item = modules->next; item; item = item->next) {
    module = (GobredModule *)item->data;
    module_definitions[i] = module->definition;
    module->definition->init();
    i++;
  }
  module_definitions[i] = NULL;
}
void
gobred_module_init_all ()
{
  if (initialized)
    return;
  gobred_module_load_all();

  initialized = true;
}

void
gobred_module_free_all ()
{
  GobredModule *module;
  for (GList *item = modules->next; item; item = item->next) {
    module = (GobredModule *)item->data;
    module->definition->free();
    g_module_close(module->gmodule);
    g_slice_free (GobredModule, module);
  }
  g_list_free(modules);
  g_free(module_definitions);

}

const GobredModuleDefinition **
gobred_module_get_all ()
{
  return module_definitions;
}


GobredModuleData *
gobred_module_data_new ()
{
  GobredModuleData *data = g_slice_new0 (GobredModuleData);
  data->magic = GOBRED_MODULE_DATA_MAGIC;
  return gobred_module_data_ref (data);

}
GobredModuleData *
gobred_module_data_ref (GobredModuleData *data)
{
  g_atomic_int_inc (&data->ref_count);
  return data;
}

void
gobred_module_data_unref (GobredModuleData *data)
{
  if (g_atomic_int_dec_and_test (&data->ref_count)) {
    g_print("freed %s\n", data->definition->name);
    g_slice_free (GobredModuleData, data);
  }
}

GobredModuleData *
gobred_module_data_from_js (JSObjectRef obj)
{
  GobredModuleData *data = (GobredModuleData *) JSObjectGetPrivate (obj);
  if (data && data->magic == GOBRED_MODULE_DATA_MAGIC)
    return data;
  else
    return NULL;
}
