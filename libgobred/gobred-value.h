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

#ifndef _GOBRED_VALUE_H_
#define _GOBRED_VALUE_H_

#include <glib.h>

typedef struct _GobredValue GobredValue;

typedef enum {
  GOBRED_VALUE_TYPE_NULL = 0x1,
  GOBRED_VALUE_TYPE_BOOLEAN = 0x2,
  GOBRED_VALUE_TYPE_NUMBER = 0x3,
  GOBRED_VALUE_TYPE_STRING = 0x4,
  GOBRED_VALUE_TYPE_ARRAY = 0x8,
  GOBRED_VALUE_TYPE_DICT = 0xf,

  GOBRED_VALUE_TYPE_END = 0x0
} GobredValueType;

struct _GobredValue {
  GobredValueType type;
  union
  {
    gboolean v_bool;
    gdouble v_number;
    gchar *v_string;
    GPtrArray *v_array;
    GHashTable *v_dict;
  } data;

  gint ref;
  gint floating;
};


GobredValue *
gobred_value_ref (GobredValue *value);

GobredValue *
gobred_value_ref_sink (GobredValue *value);

void
gobred_value_unref (GobredValue *value);

void
gobred_value_free (GobredValue *value);

GobredValueType
gobred_value_get_value_type (GobredValue *value);

GobredValue *
gobred_value_new_null ();

static inline gboolean
gobred_value_is_null (GobredValue *value)
{
  return value->type == GOBRED_VALUE_TYPE_NULL;
}

GobredValue *
gobred_value_new_boolean (gboolean b);

static inline gboolean
gobred_value_is_boolean (GobredValue *value)
{
  return value->type == GOBRED_VALUE_TYPE_BOOLEAN;
}

gboolean
gobred_value_get_boolean (GobredValue *value);

GobredValue *
gobred_value_new_number (gdouble n);

gdouble
gobred_value_get_number (GobredValue *value);

static inline gboolean
gobred_value_is_number (GobredValue *value)
{
  return value->type == GOBRED_VALUE_TYPE_NUMBER;
}

GobredValue *
gobred_value_new_string (const gchar *s);

GobredValue *
gobred_value_new_take_string (gchar *s);

const gchar *
gobred_value_get_string (GobredValue *value);

gchar *
gobred_value_take_string (GobredValue *value);

static inline gboolean
gobred_value_is_string (GobredValue *value)
{
  return value->type == GOBRED_VALUE_TYPE_STRING;
}

//////////////////////// ARRAY
//
GobredValue *
gobred_value_new_array (gsize size, ...);

static inline gboolean
gobred_value_is_array (GobredValue *value)
{
  return value->type == GOBRED_VALUE_TYPE_ARRAY;
}

gint
gobred_value_get_length (GobredValue *value);

void
gobred_value_add_item (GobredValue *value, GobredValue *item);

static inline void
gobred_value_add_null_item (GobredValue *value)
{
  gobred_value_add_item (value, gobred_value_new_null ());
}

static inline void
gobred_value_add_boolean_item (GobredValue *value, gboolean b)
{
  gobred_value_add_item (value, gobred_value_new_boolean (b));
}

static inline void
gobred_value_add_number_item (GobredValue *value, gdouble n)
{
  gobred_value_add_item (value, gobred_value_new_number (n));
}

static inline void
gobred_value_add_string_item (GobredValue *value, const gchar *s)
{
  gobred_value_add_item (value, gobred_value_new_string (s));
}

void
gobred_value_set_item (GobredValue *value, gint index, GobredValue *item);

GobredValue *
gobred_value_get_item (GobredValue *value, gint index);

static inline gboolean
gobred_value_item_is_null (GobredValue *value, gint index)
{
  return gobred_value_is_null (gobred_value_get_item (value, index));
}

static inline gboolean
gobred_value_get_boolean_item (GobredValue *value, gint index)
{
  return gobred_value_get_boolean (gobred_value_get_item (value, index));
}

static inline gdouble
gobred_value_get_number_item (GobredValue *value, gint index)
{
  return gobred_value_get_number (gobred_value_get_item (value, index));
}

static inline const gchar *
gobred_value_get_string_item (GobredValue *value, gint index)
{
  return gobred_value_get_string (gobred_value_get_item (value, index));
}

static inline gchar *
gobred_value_take_string_item (GobredValue *value, gint index)
{
  return gobred_value_take_string (gobred_value_get_item (value, index));
}



static inline GobredValueType
gobred_value_get_item_type (GobredValue *value, gint index)
{
  return gobred_value_get_item (value, index)->type;
}


/////////////////////////// DICT
//

GobredValue *
gobred_value_new_dict (const gchar *name, ...);

static inline gboolean
gobred_value_is_dict (GobredValue *value)
{
  return value->type == GOBRED_VALUE_TYPE_DICT;
}

const GobredValue *
gobred_value_get_property (GobredValue *value, const gchar *prop_name);

void
gobred_value_set_property (GobredValue *value,
			   const gchar *prop_name,
			   GobredValue *prop_value);

#endif
