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

#ifndef _LOG_H
# define _LOG_H
#include <module.h>

/*
 * Use to log errors. Don't put identifiers at the beginning. You don't have to
 * put a newline character at the end.
 * Example Usage:
 * -- example.c:
 * log_error(QUERY, "Couldn't connect.");
 * -- Output to the Console:
 * [TACK]: [QUERY]: Couldn't connect.\n
 */
extern void log_error(enum module module, char *errmsg, ...);

#endif /* _LOG_H */
