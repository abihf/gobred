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

JSValueRef
js_value_from_string (JSContextRef ctx, const char *string)
{
  JSStringRef js_string = JSStringCreateWithUTF8CString (string);
  JSValueRef val = JSValueMakeString (ctx, js_string);
  JSStringRelease (js_string);
  return val;
}

gchar *
js_string_to_native (JSContextRef ctx, JSStringRef js_string)
{
  size_t len = JSStringGetMaximumUTF8CStringSize (js_string);
  gchar *string = g_new(gchar, len);
  JSStringGetUTF8CString (js_string, string, len);
  return string;
}

gchar *
js_value_get_string (JSContextRef ctx, JSValueRef value)
{
  if (!JSValueIsString (ctx, value))
    return NULL;

  JSStringRef js_string = JSValueToStringCopy (ctx, value, NULL);
  if (js_string == NULL)
    return NULL;

  gchar *string = js_string_to_native (ctx, js_string);
  JSStringRelease (js_string);
  return string;
}

JSObjectRef
js_object_new_error (JSContextRef ctx, const gchar *format, ...)
{
  va_list arg;
  va_start(arg, format);
  JSObjectRef obj = js_object_new_errorv (ctx, format, arg);
  va_end(arg);
  return obj;
}

JSObjectRef
js_object_new_errorv (JSContextRef ctx, const gchar *format, va_list arg)
{
  JSValueRef arguments[1];
  gchar *msg = g_strdup_vprintf (format, arg);
  arguments[0] = js_value_from_string (ctx, msg);
  g_free (msg);

  return JSObjectMakeError (ctx, 1, arguments, NULL);
}

