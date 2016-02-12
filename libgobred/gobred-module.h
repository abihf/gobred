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



#ifndef _GOBRED_MODULES_H_
#define _GOBRED_MODULES_H_

typedef enum {
	GOBRED_MODULE_DEFINITION_VERSION_0 = 0
} GobredModuleDefinitionVersion;

typedef struct
{
  GobredModuleDefinitionVersion version;
  gchar *name;
} GobredModuleDefinitionBase;

typedef struct
{
  GobredModuleDefinitionBase base;
  GobredMethodDefinitionV0 *methods;
  GobredEventDefinitionV0 *events;

  void (*init) (void);
  void (*free) (void);

} GobredModuleDefinitionV0;

typedef const GobredModuleDefinitionBase *(*GobredModuleRegisterFunc)();


#endif
