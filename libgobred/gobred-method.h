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

#ifndef _GOBRED_METHOD_H_
#define _GOBRED_METHOD_H_

#include <glib.h>

#ifndef JavaScript_h // if JavaScriptCore not defined
typedef const void *JSValueRef;
typedef const void *JSContextRef;
#endif

typedef enum
{
  GOBRED_METHOD_TYPE_SIMPLE = 0,
  GOBRED_METHOD_TYPE_NATIVE = 1
} GobredMethodType;

typedef struct _GobredMethodCallBack GobredMethodCallBack;
typedef struct _GobredMethodDefinitionV0 GobredMethodDefinitionV0;

typedef void
(*GobredMethodSimpleHandler) (GobredArray *params, GobredMethodCallBack *cb);

typedef JSValueRef
(*GobredMethodNativeHander) (JSContextRef ctx,
			     gsize argc,
			     const JSValueRef js_args[],
			     JSValueRef *exception);



void
gobred_method_callback_throw_error (GobredMethodCallBack **mcb,
				    const gchar *format,
				    ...) G_GNUC_PRINTF(2, 3);

void
gobred_method_callback_return (GobredMethodCallBack **mcb, GobredValue *value);


struct _GobredMethodDefinitionV0
{
  gchar *name;
  GobredMethodType type;
  union
  {
    GobredMethodSimpleHandler simple;
    GobredMethodNativeHander native;
  } handler;
  gboolean threaded;
  gpointer data;
};

#endif
