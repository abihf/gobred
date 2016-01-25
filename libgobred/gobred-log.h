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

#ifndef _GOBRED_LOG_H_
#define _GOBRED_LOG_H_

// use g_debug, g_info, and g_message for debuging purpose
typedef enum {
  GOBRED_ERROR_LEVEL_DEBUG     = 1,
  GOBRED_ERROR_LEVEL_INFO,
  GOBRED_ERROR_LEVEL_MESSAGE,
  GOBRED_ERROR_LEVEL_WARNING   = 4,
  GOBRED_ERROR_LEVEL_CRITICAL,
  GOBRED_ERROR_LEVEL_ERROR,
  GOBRED_ERROR_LEVEL_FATAL,
} GobredErrorLevel;

void
_gobred_log(GobredErrorLevel level, const gchar *msg);

void
_gobred_error(GobredErrorLevel level,
	      guint64 code,
	      const gchar *name);

#define gobred_warning(msg, ...) \
	_gobred_log(GOBRED_ERROR_LEVEL_WARNING, msg, ...)

#define gobred_critical(name) \
	_gobred_error(GOBRED_ERROR_LEVEL_CRITICAL, name, "#name#")

#define gobred_error(name) \
	_gobred_error(GOBRED_ERROR_LEVEL_ERROR, name, "#name")

#define gobred_fatal(name) \
	_gobred_error(GOBRED_ERROR_LEVEL_FATAL, name, "#name")

#endif
