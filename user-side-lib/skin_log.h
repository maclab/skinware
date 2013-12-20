/*
 * Copyright (C) 2011-2013  Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * The research leading to these results has received funding from
 * the European Commission's Seventh Framework Programme (FP7) under
 * Grant Agreement n. 231500 (ROBOSKIN).
 *
 * This file is part of Skinware.
 *
 * Skinware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Skinware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Skinware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SKIN_LOG_H
#define SKIN_LOG_H

#include "skin_config.h"
#include "skin_compiler.h"

struct skin_object;

SKIN_DECL_BEGIN

/*
 * Log functions:
 *
 * init				open the log file with name skin.log (or skin-??.log if not possible)
 * skin_log			log the message to file, automatically appending new line
 * no_newline			same as skin_log, but without new line.  The buffer will be flushed
 * set_error			same as skin_log as well as setting the error message in skin_object (without new line)
 * free				close the log file
 */
int skin_log_init(void);
#if SKIN_DEBUG
# define	skin_log(fmt, ...)			skin_log_c(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
# define	skin_log_no_newline(fmt, ...)		skin_log_no_newline_c(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
# define	skin_log_set_error(o, fmt, ...)		skin_log_set_error_c(o, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
# define	skin_log(fmt, ...)			skin_log_c(NULL, 0, fmt, ##__VA_ARGS__)
# define	skin_log_no_newline(fmt, ...)		skin_log_no_newline_c(NULL, 0, fmt, ##__VA_ARGS__)
# define	skin_log_set_error(o, fmt, ...)		skin_log_set_error_c(o, NULL, 0, fmt, ##__VA_ARGS__)
#endif
void skin_log_c(const char *file_name, unsigned int line,
		const char *fmt, ...) SKIN_PRINTF_STYLE(3, 4);
void skin_log_no_newline_c(const char *file_name, unsigned int line,
		const char *fmt, ...) SKIN_PRINTF_STYLE(3, 4);
void skin_log_set_error_c(struct skin_object *so, const char *file_name, unsigned int line,
		const char *fmt, ...) SKIN_PRINTF_STYLE(4, 5);
void skin_log_flush(void);
void skin_log_free(void);

SKIN_DECL_END

#endif
