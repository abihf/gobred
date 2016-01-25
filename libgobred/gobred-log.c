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
#include <stdio.h>
#include "gobred-log.h"


void
_gobred_error(GobredErrorLevel level,
	      guint64 code,
	      const gchar *name)
{
  const gchar *level_name;
  switch (level) {
  case GOBRED_ERROR_LEVEL_CRITICAL:
  case GOBRED_ERROR_LEVEL_ERROR:
  case GOBRED_ERROR_LEVEL_FATAL:
    level_name = "ERROR";
    break;

  default:
    level_name = "WARNING";
    break;

  }
  fprintf(stderr, "%s %08lx: %s", level_name, code, name);
}
