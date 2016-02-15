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

struct _GobredMethodCallBack
{
  JSContextRef ctx;
  JSObjectRef js_func;
  JSObjectRef this_object;
};

typedef struct {
  GobredMethodSimpleHandler handler;
  GobredValue *params;
  GobredMethodCallBack *cb;
} SimpleMethodHandlerData;

static gpointer
_method_handler_thread (gpointer pdata)
{
  SimpleMethodHandlerData *data = (SimpleMethodHandlerData*)pdata;
  gobred_value_ref_sink (data->params);
  data->handler(data->params, data->cb);
  gobred_value_unref (data->params);
  g_slice_free (SimpleMethodHandlerData, data);
  return NULL;
}

static JSValueRef
gobred_method_handle_call_simple (GobredMethodSimpleHandler handler,
				                          JSContextRef ctx,
				                          size_t argc,
				                          const JSValueRef js_args[],
                                  gboolean threaded,
				                          JSValueRef* exception)
{
  JSValueRef tmp_exception = NULL;
  int i;

  GobredMethodCallBack *cb = NULL;
  if (argc >= 1 && JSValueIsObject (ctx, js_args[argc - 1])) {
    JSObjectRef js_func = JSValueToObject (ctx, js_args[argc - 1], &tmp_exception);
    if (JSObjectIsFunction (ctx, js_func)) {
      cb = g_slice_new(GobredMethodCallBack);
      cb->js_func = js_func;
      cb->ctx = JSContextGetGlobalContext(ctx);
    }
  }

  // TODO: call JSObjectIsFunction(ctx, js_func) and throw

  int real_argc = argc > 0 ? argc - 1 : 0;
  GobredValue *params = gobred_value_new_array (real_argc, 0);
  for (i = 0; i < real_argc; i++) {
    gobred_value_add_item (params, gobred_value_new_from_js (ctx, js_args[i]));
  }

  if (threaded) {
    SimpleMethodHandlerData *data = g_slice_new (SimpleMethodHandlerData);
    data->params = params;
    data->cb = cb;
    data->handler = handler;
    GThread *thread = g_thread_new ("method-handler",
                                    _method_handler_thread,
                                    data);
    g_thread_unref (thread);
  } else {
    gobred_value_ref_sink (params);
    handler (params, cb);
    gobred_value_unref (params);
  }

  return JSValueMakeUndefined (ctx);
}

static JSValueRef
gobred_method_handle_call_v0 (JSContextRef ctx,
			   JSObjectRef function,
			   JSObjectRef this_object,
			   size_t argc,
			   const JSValueRef js_args[],
			   JSValueRef* exception)
{

  const GobredMethodDefinitionV0 *method =
      (GobredMethodDefinitionV0 *) JSObjectGetPrivate (function);

  switch (method->type) {
  case GOBRED_METHOD_TYPE_SIMPLE:
    return gobred_method_handle_call_simple (method->handler.simple,
                                             ctx,
                                             argc,
                                             js_args,
                                             method->threaded,
                                             exception);

  case GOBRED_METHOD_TYPE_NATIVE:
    return method->handler.native (ctx, argc, js_args, exception);

  default:
    return JSValueMakeUndefined (ctx);
  }
}

static JSClassDefinition _method_class_definition_v0 =
  { .className = "GobredMethod", .callAsFunction = gobred_method_handle_call_v0 };
static JSClassRef _method_class_v0 = NULL;

JSObjectRef
gobred_method_create_js_func_v0 (JSContextRef ctx,
				 GobredMethodDefinitionV0 *definition,
				 JSStringRef *out_name)
{
  JSStringRef name = JSStringCreateWithUTF8CString(definition->name);
  *out_name = name;
  if (_method_class_v0 == NULL)
    _method_class_v0 = JSClassCreate (&_method_class_definition_v0);
  return JSObjectMake(ctx, _method_class_v0, definition);
}

void
gobred_method_callback_return (GobredMethodCallBack **pcb, GobredValue *value)
{
  GobredMethodCallBack *cb = *pcb;
  if (cb == NULL)
    return;

  JSValueRef params[2];
  gint param_count = 1;
  params[0] = JSValueMakeNull (cb->ctx);

  if (value) {
    gobred_value_ref_sink (value);
    params[1] = gobred_value_to_js (value, cb->ctx);
    param_count++;
    gobred_value_unref (value);
    g_print("ada return nya\n");
  }

  // JavaScriptCore API is thread safe, so we can safely do it
  JSObjectCallAsFunction (cb->ctx, cb->js_func, NULL, param_count, params,
			  NULL);

  // clear callback
  g_slice_free (GobredMethodCallBack, cb);
  *pcb = NULL;
}

void
gobred_method_callback_throw_error (GobredMethodCallBack **pcb,
				    const gchar *format,
				    ...)
{
  GobredMethodCallBack *cb = *pcb;
  if (cb == NULL)
    return;

  va_list arg;
  va_start(arg, format);

  JSValueRef params[1];
  params[0] = (JSValueRef) js_object_new_errorv (cb->ctx, format, arg);

  va_end(arg);

  JSObjectCallAsFunction (cb->ctx, cb->js_func, NULL, 1, params, NULL);
  // clear callback
  g_slice_free (GobredMethodCallBack, cb);
  *pcb = NULL;
}

