/* tack - A CLI project manager written in C
Copyright (C) 2026  Emir Baha Yıldırım

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <log.h>

void log_error(enum module module, char *errmsg, ...)
{
	const char *module_name;
	va_list args;
	char *msg = NULL;
	if (!errmsg || !module)
		return;

	va_start(args, errmsg);
	module_name = get_module_name(module);
	vasprintf(&msg, errmsg, args);
	va_end(args);
    if (msg) {
        fprintf(stderr, "[TACK]: [%s]: %s\n", module_name, msg);
		free(msg);
    }
	return;
}
