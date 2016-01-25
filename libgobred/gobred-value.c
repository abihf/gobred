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

// G_DEFINE_BOXED_TYPE(GobredValue, gobred_value, gobred_value_ref, gobred_value_unref);

static GobredValue *
gobred_value_new (GobredValueType type)
{
  GobredValue *value = g_slice_new0(GobredValue);
  value->type = type;
  value->ref = 1;
  value->floating = 1;
  return value;
}

static GobredValue *
gobred_value_newv (va_list *args)
{
  GobredValueType type = va_arg(*args, GobredValueType);
  switch (type) {

  case GOBRED_VALUE_TYPE_NULL:
    return gobred_value_new_null ();

  case GOBRED_VALUE_TYPE_BOOLEAN:
    return gobred_value_new_boolean (va_arg(*args, gboolean));

  case GOBRED_VALUE_TYPE_NUMBER:
    return gobred_value_new_number (va_arg(*args, gdouble));

  case GOBRED_VALUE_TYPE_STRING:
    return gobred_value_new_string (va_arg(*args, const gchar *));

  case GOBRED_VALUE_TYPE_ARRAY:
  case GOBRED_VALUE_TYPE_DICT:
    return va_arg(*args, GobredValue *);

  default:
    return NULL;
  }
}

GobredValue *
gobred_value_ref (GobredValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_atomic_int_inc(&value->ref);
  return value;
}

gboolean
gobred_value_is_floating (GobredValue *value)
{
  g_return_val_if_fail (value != NULL, FALSE);
  return value->floating != 0;
}

GobredValue *
gobred_value_ref_sink (GobredValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  if (gobred_value_is_floating (value))
    g_atomic_int_dec_and_test(&value->floating);
  else
    gobred_value_ref (value);
  return value;
}

void
gobred_value_unref (GobredValue *value)
{
  g_return_if_fail (value != NULL);
  if (g_atomic_int_dec_and_test(&value->ref)) {
    gobred_value_free (value);
  }
}

GobredValueType
gobred_value_get_value_type (GobredValue *value)
{
  return value->type;
}

void
gobred_value_free (GobredValue *value)
{
  switch (value->type) {
  case GOBRED_VALUE_TYPE_STRING:
    g_free (value->data.v_string);
    break;

  case GOBRED_VALUE_TYPE_ARRAY:
    g_ptr_array_unref (value->data.v_array);
    break;

  case GOBRED_VALUE_TYPE_DICT:
    g_hash_table_unref (value->data.v_dict);
    break;

  default:
    break;
  }
  g_slice_free(GobredValue, value);
}

static JSValueRef
gobred_value_array_to_js (GobredValue *value, JSContextRef ctx)
{
  int i, len = value->data.v_array->len;
  gpointer *item = value->data.v_array->pdata;
  JSValueRef params[] = {NULL};
  JSObjectRef array = JSObjectMakeArray (ctx, 0, params, NULL);

  for (i = 0; i < len; i++) {
    JSValueRef js_item = gobred_value_to_js ((GobredValue*) *item, ctx);
    JSObjectSetPropertyAtIndex (ctx, array, i, js_item, NULL);
    item++;
  }
  return array;
}

static JSValueRef
gobred_value_dict_to_js (GobredValue *value, JSContextRef ctx)
{
  GHashTableIter iter;
  const gchar *name;
  GobredValue *prop;
  JSObjectRef obj = JSObjectMake (ctx, NULL, NULL);
  g_hash_table_iter_init (&iter, value->data.v_dict);
  while (g_hash_table_iter_next (&iter, (gpointer *) &name, (gpointer *) &prop)) {
    JSStringRef js_name = JSStringCreateWithUTF8CString (name);
    JSValueRef js_prop = gobred_value_to_js (prop, ctx);
    JSObjectSetProperty (ctx, obj, js_name, js_prop, 0, NULL);
    JSStringRelease (js_name);
  }
  return obj;
}

JSValueRef
gobred_value_to_js (GobredValue *value, JSContextRef ctx)
{
  switch (value->type) {
  case GOBRED_VALUE_TYPE_NULL:
    return JSValueMakeNull (ctx);

  case GOBRED_VALUE_TYPE_BOOLEAN:
    return JSValueMakeBoolean (ctx, gobred_value_get_boolean (value));

  case GOBRED_VALUE_TYPE_NUMBER:
    return JSValueMakeNumber (ctx, gobred_value_get_number (value));

  case GOBRED_VALUE_TYPE_STRING:
    return js_value_from_string (ctx, gobred_value_get_string (value));

  case GOBRED_VALUE_TYPE_ARRAY:
    return gobred_value_array_to_js (value, ctx);

  case GOBRED_VALUE_TYPE_DICT:
    return gobred_value_dict_to_js (value, ctx);

  default:
    return JSValueMakeUndefined (ctx);
  }
}

static GobredValue *
gobred_value_new_from_js_object (JSContextRef ctx, JSObjectRef js_object)
{
  if (JSValueIsArray (ctx, js_object)) {
    JSStringRef js_len_name = JSStringCreateWithUTF8CString ("length");
    JSValueRef js_len = JSObjectGetProperty (ctx, js_object, js_len_name, NULL);
    gint len = (gint) JSValueToNumber (ctx, js_len, NULL);
    JSStringRelease (js_len_name);
    GobredValue *array = gobred_value_new_array (len, 0);
    for (int i = 0; i < len; i++) {
      JSValueRef js_item = JSObjectGetPropertyAtIndex (ctx, js_object, i, NULL);
      gobred_value_add_item (array, gobred_value_new_from_js (ctx, js_item));
    }
    return array;
  }
  else {
    JSPropertyNameArrayRef props = JSObjectCopyPropertyNames (ctx, js_object);
    size_t len = JSPropertyNameArrayGetCount (props);
    int i;

    GobredValue *value = gobred_value_new_dict (NULL);
    for (i = 0; i < len; i++) {
      JSStringRef js_prop_name = JSPropertyNameArrayGetNameAtIndex (props, i);
      JSValueRef js_prop_value = JSObjectGetProperty (ctx, js_object,
						      js_prop_name,
						      NULL);
      gchar *prop_name = js_string_to_native (ctx, js_prop_name);
      GobredValue *prop_value = gobred_value_new_from_js (ctx, js_prop_value);
      gobred_value_set_property (value, prop_name, prop_value);

      g_free (prop_name);
      JSStringRelease (js_prop_name);
    }
    JSPropertyNameArrayRelease (props);
    return value;
  }
  return NULL;
}

GobredValue *
gobred_value_new_from_js (JSContextRef ctx, JSValueRef js_value)
{
  switch (JSValueGetType (ctx, js_value)) {
  case kJSTypeUndefined:
  case kJSTypeNull:
    return gobred_value_new_null ();

  case kJSTypeBoolean:
    return gobred_value_new_boolean (JSValueToBoolean (ctx, js_value));

  case kJSTypeNumber:
    return gobred_value_new_number (JSValueToNumber (ctx, js_value,
    NULL));

  case kJSTypeString:
    return gobred_value_new_take_string (js_value_get_string (ctx, js_value));

  case kJSTypeObject:
    return gobred_value_new_from_js_object (ctx, JSValueToObject (ctx, js_value,
    NULL));
  }
  return NULL;
}

GobredValue *
gobred_value_new_null ()
{
  return gobred_value_new (GOBRED_VALUE_TYPE_NULL);
}

GobredValue *
gobred_value_new_boolean (gboolean b)
{
  GobredValue *value = gobred_value_new (GOBRED_VALUE_TYPE_BOOLEAN);
  value->data.v_bool = b;
  return value;
}

gboolean
gobred_value_get_boolean (GobredValue *value)
{
  g_return_val_if_fail(gobred_value_is_boolean (value), FALSE);
  return value->data.v_bool;
}

GobredValue *
gobred_value_new_number (gdouble n)
{
  GobredValue *value = gobred_value_new (GOBRED_VALUE_TYPE_NUMBER);
  value->data.v_number = n;
  return value;
}

gdouble
gobred_value_get_number (GobredValue *value)
{
  g_return_val_if_fail(gobred_value_is_number (value), 0.0);
  return value->data.v_number;
}

GobredValue *
gobred_value_new_take_string (gchar *s)
{
  GobredValue *value = gobred_value_new (GOBRED_VALUE_TYPE_STRING);
  value->data.v_string = g_strdup (s);
  return value;
}

GobredValue *
gobred_value_new_string (const gchar *s)
{
  return gobred_value_new_take_string (g_strdup (s));
}

const gchar *
gobred_value_get_string (GobredValue *value)
{
  g_return_val_if_fail(gobred_value_is_string (value), NULL);
  return value->data.v_string;
}

gchar *
gobred_value_take_string (GobredValue *value)
{
  return g_strdup (gobred_value_get_string (value));
}

//////////////////////// ARRAY

GobredValue *
gobred_value_new_array (gsize size, ...)
{
  GobredValue *value = gobred_value_new (GOBRED_VALUE_TYPE_ARRAY);
  if (size > 0)
    value->data.v_array = g_ptr_array_new_full (
	size, (GDestroyNotify) gobred_value_unref);
  else
    value->data.v_array = g_ptr_array_new_with_free_func (
	(GDestroyNotify) gobred_value_unref);

  va_list args;
  va_start(args, size);
  GobredValue *item;
  while ((item = gobred_value_newv (&args)) != NULL) {
    gobred_value_add_item (value, item);
  }
  va_end(args);
  return value;
}

gint
gobred_value_get_length (GobredValue *value)
{
  g_return_val_if_fail(gobred_value_is_array (value), 0);
  return value->data.v_array->len;
}

void
gobred_value_add_item (GobredValue *value, GobredValue *item)
{
  g_return_if_fail(gobred_value_is_array (value));
  g_ptr_array_add (value->data.v_array, gobred_value_ref_sink (item));
}

void
gobred_value_set_item (GobredValue *value, gint index, GobredValue *item)
{
  g_return_if_fail(gobred_value_is_array (value));
  GPtrArray *array = value->data.v_array;
  if (array->len <= index)
    g_ptr_array_set_size (array, index + 1);
  if (array->pdata[index])
    gobred_value_unref (array->pdata[index]);
  array->pdata[index] = gobred_value_ref_sink (item);
}

GobredValue *
gobred_value_get_item (GobredValue *value, gint index)
{
  g_return_val_if_fail(gobred_value_is_array (value), NULL);
  GPtrArray *array = value->data.v_array;
  if (index < array->len)
    return (GobredValue *) array->pdata[index];
  else
    return NULL;
}

GobredValue *
gobred_value_new_dict (const gchar *name, ...)
{

  GobredValue *value = gobred_value_new (GOBRED_VALUE_TYPE_DICT);
  value->data.v_dict = g_hash_table_new_full (
      (GHashFunc) g_str_hash, (GEqualFunc) g_str_equal, (GDestroyNotify) g_free,
      (GDestroyNotify) gobred_value_unref);
  const gchar *prop_name;
  va_list args;
  va_start(args, name);
  for (prop_name = name; prop_name != NULL;
      prop_name = va_arg(args, const gchar*)) {
    GobredValue *prop_value = gobred_value_newv (&args);
    //va_arg(args, GobredValueType);
    //va_arg(args, const gchar *);
    gobred_value_set_property (value, prop_name, prop_value);
  }
  va_end(args);

  return value;
}

void
gobred_value_set_property (GobredValue *value,
			   const gchar *prop_name,
			   GobredValue *prop_value)
{
  g_return_if_fail(gobred_value_is_dict (value));
  g_hash_table_insert (value->data.v_dict, g_strdup (prop_name),
		       gobred_value_ref_sink (prop_value));
}

const GobredValue *
gobred_value_get_property (GobredValue *value, const gchar *prop_name)
{
  g_return_val_if_fail(gobred_value_is_dict (value), NULL);
  return g_hash_table_lookup (value->data.v_dict, prop_name);
}

