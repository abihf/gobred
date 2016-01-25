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

#ifndef _GOBRED_UTILS_H_
#define _GOBRED_UTILS_H_

JSValueRef
js_value_from_string (JSContextRef ctx, const char *string);

gchar *
js_string_to_native (JSContextRef ctx, JSStringRef js_string);

gchar *
js_value_get_string (JSContextRef ctx, JSValueRef value);

JSObjectRef
js_object_new_error (JSContextRef ctx, const gchar *format, ...) G_GNUC_PRINTF(2, 3);

JSObjectRef
js_object_new_errorv (JSContextRef ctx, const gchar *format, va_list arg);

#endif
