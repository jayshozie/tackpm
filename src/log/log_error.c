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
#include <time.h>
#include <log/log.h>

void log_error(enum module module, char *errmsg, ...)
{
	const char *module_name;
	va_list args;
	FILE *log_file;
	time_t rawtime;
	struct tm *timeinfo;
	char timestamp[20];

	if (!errmsg || !module)
		return;

	/* Using /tmp to ensure writability for diagnostics */
	log_file = fopen("/tmp/tack.log", "a");
	if (!log_file)
		return;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

	module_name = get_module_name(module);

	fprintf(log_file, "[%s] [TACK]: [%s]: ", timestamp, module_name);
	va_start(args, errmsg);
	vfprintf(log_file, errmsg, args);
	va_end(args);
	fprintf(log_file, "\n");

	fclose(log_file);
}
