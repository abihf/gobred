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

/*
GOBRED_INTERNAL JSValueRef
gobred_method_handle_call (JSContextRef ctx,
			   JSObjectRef function,
			   JSObjectRef this_object,
			   size_t argc,
			   const JSValueRef js_args[],
			   JSValueRef* exception);
*/

JSObjectRef
gobred_method_create_js_func_v0 (JSContextRef ctx,
				 GobredMethodDefinitionV0 *definition,
				 JSStringRef *out_name);
