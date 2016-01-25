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
static JSClassRef _module_class = NULL;
static JSClassRef _method_class = NULL;

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

static void
_module_finalize (JSObjectRef object)
{
  GobredModuleData *data = gobred_module_data_from_js (object);
  g_print ("finalizing %d\n", data!=NULL);
  if (data) {
    data->active = false;
    gobred_module_data_unref (data);
  }
}

static JSStaticFunction _moudule_static_functions[] = {
  {
    .name = "addEventListener",
    .callAsFunction = gobred_event_handle_add_listener,
    .attributes = kJSPropertyAttributeReadOnly
  },
  {
    .name = "removeEventListener",
    .callAsFunction = gobred_event_handle_add_listener,
    .attributes = kJSPropertyAttributeReadOnly
  },
  { NULL }
};

static JSClassDefinition _module_class_definition = {
  .className = "GobredModule",
  .finalize = _module_finalize,
  .staticFunctions = _moudule_static_functions
};

static JSClassDefinition _method_class_definition =
  { .className = "GobredMethod", .callAsFunction = gobred_method_handle_call };

static JSObjectRef
create_gobred_object (JSContextRef ctx)
{
  //GobredObjectData *data = g_slice_new(GobredObjectData, 1);
  //data->magic = GOBRED_MAGIC;
  //data->event_container = gobred_event_container_new ();

  JSObjectRef object = JSObjectMake (ctx, _gobred_class, NULL);

  // register modules & methods
  const GobredModuleDefinition *module;
  const GobredModuleDefinition **modules = gobred_module_get_all ();
  for (int i = 0; modules[i]; i++) {
    module = modules[i];
    JSStringRef module_name = JSStringCreateWithUTF8CString (module->name);
    GobredModuleData *data = gobred_module_data_new ();
    data->active = true;
    data->definition = module;
    JSObjectRef obj_module = JSObjectMake (ctx, _module_class, data);

    GobredMethodDefinition *method;
    for (method = module->methods; method->name; method++) {
      JSStringRef method_name = JSStringCreateWithUTF8CString (method->name);
      JSObjectRef fun = JSObjectMake (ctx, _method_class, method);
      JSObjectSetProperty (ctx, obj_module, method_name, fun, 0, NULL);
      JSStringRelease (method_name);
      JSValueUnprotect (ctx, fun);
    }

    JSObjectSetProperty (ctx, object, module_name, obj_module, 0, NULL);
    JSStringRelease (module_name);
    JSValueUnprotect (ctx, obj_module);
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
  _module_class = JSClassCreate (&_module_class_definition);
  _method_class = JSClassCreate (&_method_class_definition);
  gobred_module_init_all ();
  gobred_event_prepare (gobred_module_get_all());
}

void
gobred_bridge_end ()
{
  g_print ("Bridge cleared\n");
  gobred_module_free_all ();
  gobred_event_clean();
  JSClassRelease (_gobred_class);
  JSClassRelease (_module_class);
  JSClassRelease (_method_class);
}

