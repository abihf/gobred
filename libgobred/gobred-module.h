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

#define GOBRED_MODULE_MAX_NAME  128


typedef struct
{
  gchar *name;
  GobredMethodDefinition *methods;
  GobredEventDefinition *events;

  void (*init) (void);
  void (*free) (void);

} GobredModuleDefinition;

typedef const GobredModuleDefinition *(*GobredModuleRegisterFunc)();


#endif
