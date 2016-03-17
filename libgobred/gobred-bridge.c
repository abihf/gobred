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
#include "config.h"
#include "gobred.h"
#include "gobred-private.h"
#include "gobred-js-utils.h"

static JSClassRef _gobred_class = NULL;

static JSValueRef
get_version_cb (JSContextRef ctx,
		JSObjectRef object,
		JSStringRef propertyName,
		JSValueRef* exception)
{
  return js_value_from_string (ctx, PACKAGE_STRING);
}


static JSStaticValue _gobred_static_values[] = {
  {
    .name = "VERSION",
    .getProperty = get_version_cb,
    .attributes = kJSPropertyAttributeReadOnly
  },
  { NULL }
};


static JSClassDefinition _gobred_class_definition = {
  .className = "Gobred",
  .staticValues = _gobred_static_values
};

static JSObjectRef
create_gobred_object (JSContextRef ctx)
{
  //GobredObjectData *data = g_slice_new(GobredObjectData, 1);
  //data->magic = GOBRED_MAGIC;
  //data->event_container = gobred_event_container_new ();

  JSObjectRef object = JSObjectMake (ctx, _gobred_class, NULL);

  // register modules & methods
  const GList *modules = gobred_module_get_all();
  for (GList *item = modules->next; item; item = item->next) {
    GobredModule *module = (GobredModule *)item->data;
    JSObjectRef module_js = gobred_module_create_js_object (ctx, module);
    JSStringRef name = JSStringCreateWithUTF8CString(module->definition->name);
    JSObjectSetProperty (ctx, object, name, module_js, 0, NULL);
    JSValueUnprotect (ctx, module_js);
    JSStringRelease (name);
  }
  return object;
}

void
gobred_bridge_setup (JSContextRef context)
{
  JSObjectRef global_object = JSContextGetGlobalObject (context);

  JSStringRef prop_name = JSStringCreateWithUTF8CString ("gobred");
  JSObjectRef gobred = create_gobred_object (context);
  JSObjectSetProperty (context, global_object, prop_name, gobred,
		       kJSPropertyAttributeReadOnly, NULL);
  JSStringRelease (prop_name);
  JSValueUnprotect (context, gobred);
  //JSValueUnprotect (context, global_object);
  //JSGarbageCollect (context);
}

void
gobred_bridge_init ()
{
  _gobred_class = JSClassCreate (&_gobred_class_definition);
  gobred_module_init_all ();
}

void
gobred_bridge_end ()
{
  g_print ("Bridge cleared\n");
  gobred_module_free_all ();
  gobred_event_clean();
  JSClassRelease (_gobred_class);
}

