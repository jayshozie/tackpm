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

/* clang-format off */
#ifndef _MODULE_H
#  define _MODULE_H
/* clang-format on */
#include <stdint.h>

enum module {
	LOG,
	BCKEND,
	MENU,
	MAIN,
	STATE,
	OTHER,
};

struct Module {
	enum module id;
	char *name;
};

extern char *get_module_name(enum module module_id);

#endif /* _MODULE_H */
