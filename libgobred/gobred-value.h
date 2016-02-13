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
typedef GobredValue GobredArray;
typedef GobredValue GobredDict;


extern GobredValue _gobred_boolean_true;
#define GOBRED_BOOLEAN_TRUE &_gobred_boolean_true

extern GobredValue _gobred_boolean_false;
#define GOBRED_BOOLEAN_FALSE &_gobred_boolean_false


typedef enum {
  GOBRED_VALUE_TYPE_NULL = 0x1,
  GOBRED_VALUE_TYPE_BOOLEAN = 0x2,
  GOBRED_VALUE_TYPE_NUMBER = 0x3,
  GOBRED_VALUE_TYPE_STRING = 0x4,
  GOBRED_VALUE_TYPE_ARRAY = 0x8,
  GOBRED_VALUE_TYPE_DICT = 0xf,

  GOBRED_VALUE_TYPE_END = 0x0,
  GOBRED_VALUE_TYPE_UNKNOWN = 0x0
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
  gboolean constant;
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
  return gobred_value_get_value_type(value) == GOBRED_VALUE_TYPE_NULL;
}

GobredValue *
gobred_value_new_boolean (gboolean b);

static inline gboolean
gobred_value_is_boolean (GobredValue *value)
{
  return gobred_value_get_value_type(value) == GOBRED_VALUE_TYPE_BOOLEAN;
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
  return gobred_value_get_value_type(value) == GOBRED_VALUE_TYPE_NUMBER;
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
  return gobred_value_get_value_type(value) == GOBRED_VALUE_TYPE_STRING;
}

//////////////////////// ARRAY
//
GobredArray *
gobred_array_new (gsize size, ...);

static inline gboolean
gobred_value_is_array (GobredValue *value)
{
  return gobred_value_get_value_type(value) == GOBRED_VALUE_TYPE_ARRAY;
}

gint
gobred_array_get_length (GobredArray *array);

void
gobred_array_add (GobredArray *array, GobredValue *item);

static inline void
gobred_array_add_null (GobredArray *array)
{
  gobred_array_add (array, gobred_value_new_null ());
}

static inline void
gobred_array_add_boolean (GobredArray *array, gboolean b)
{
  gobred_array_add (array, gobred_value_new_boolean (b));
}

static inline void
gobred_array_add_number (GobredArray *array, gdouble n)
{
  gobred_array_add (array, gobred_value_new_number (n));
}

static inline void
gobred_array_add_string (GobredArray *array, const gchar *s)
{
  gobred_array_add (array, gobred_value_new_string (s));
}

#define gobred_array_add_array gobred_array_add
#define gobred_array_add_dict gobred_array_add

void
gobred_array_set (GobredArray *array, gint index, GobredValue *item);

GobredValue *
gobred_array_get (GobredArray *array, gint index);

static inline gboolean
gobred_array_item_is_null (GobredArray *array, gint index)
{
  return gobred_value_is_null (gobred_array_get (array, index));
}

static inline gboolean
gobred_array_get_boolean (GobredArray *array, gint index)
{
  return gobred_value_get_boolean (gobred_array_get (array, index));
}

static inline gdouble
gobred_array_get_number (GobredArray *array, gint index)
{
  return gobred_value_get_number (gobred_array_get (array, index));
}

static inline const gchar *
gobred_array_get_string (GobredArray *array, gint index)
{
  return gobred_value_get_string (gobred_array_get (array, index));
}

static inline gchar *
gobred_array_take_string (GobredArray *array, gint index)
{
  return gobred_value_take_string (gobred_array_get (array, index));
}

#define gobred_array_get_array gobred_array_get
#define gobred_array_get_dict gobred_array_get

static inline GobredValueType
gobred_array_get_item_type (GobredArray *array, gint index)
{
  return gobred_value_get_value_type(gobred_array_get (array, index));
}


/////////////////////////// DICT
//

GobredDict *
gobred_dict_new (const gchar *name, ...);

static inline gboolean
gobred_value_is_dict (GobredValue *value)
{
  return gobred_value_get_value_type(value) == GOBRED_VALUE_TYPE_DICT;
}

GobredValue *
gobred_dict_get (GobredDict *dict, const gchar *prop_name);

static inline gboolean
gobred_dict_get_boolean (GobredDict *dict,
			 const gchar *prop_name,
			 gboolean default_value)
{
  GobredValue *prop = gobred_dict_get(dict, prop_name);
  if (prop)
    return gobred_value_get_boolean (prop);
  else
    return default_value;
}

static inline gdouble
gobred_dict_get_number (GobredDict *dict,
			 const gchar *prop_name,
			 gdouble default_value)
{
  GobredValue *prop = gobred_dict_get(dict, prop_name);
  if (prop)
    return gobred_value_get_number (prop);
  else
    return default_value;
}

static inline const gchar *
gobred_dict_get_string (GobredDict *dict,
			 const gchar *prop_name,
			 const gchar * default_value)
{
  GobredValue *prop = gobred_dict_get(dict, prop_name);
  if (prop)
    return gobred_value_get_string (prop);
  else
    return default_value;
}

static inline gchar *
gobred_dict_take_string (GobredDict *dict,
			 const gchar *prop_name,
			 gchar * default_value)
{
  GobredValue *prop = gobred_dict_get(dict, prop_name);
  if (prop)
    return gobred_value_take_string (prop);
  else
    return default_value;
}


void
gobred_dict_set (GobredDict *dict,
			   const gchar *prop_name,
			   GobredValue *prop_value);

static inline void
gobred_dict_set_null (GobredDict *dict, const gchar *prop_name)
{
  gobred_dict_set(dict, prop_name, NULL);
}

static inline void
gobred_dict_set_boolean (GobredDict *dict,
		   const gchar *prop_name,
		   gboolean value)
{
  gobred_dict_set(dict, prop_name,
		  value ? GOBRED_BOOLEAN_TRUE : GOBRED_BOOLEAN_FALSE);
}


#endif
