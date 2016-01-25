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

GOBRED_INTERNAL const GobredModuleDefinition **
gobred_module_get_all ();

GOBRED_INTERNAL void
gobred_module_init_all ();

GOBRED_INTERNAL void
gobred_module_free_all ();


// JSObject user-data structure for module object

typedef struct {
  guint32 magic;
  const GobredModuleDefinition *definition;
  gboolean active;
  gboolean ref_count;
} GobredModuleData;

#define GOBRED_MODULE_DATA_MAGIC 0xbeafdead

GOBRED_INTERNAL GobredModuleData *
gobred_module_data_new ();

GOBRED_INTERNAL GobredModuleData *
gobred_module_data_ref (GobredModuleData *data);

GOBRED_INTERNAL void
gobred_module_data_unref (GobredModuleData *data);

GOBRED_INTERNAL GobredModuleData *
gobred_module_data_from_js (JSObjectRef obj);


