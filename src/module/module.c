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

#include <module/module.h>

static struct Module modules[] = {
	{ LOG, "LOG" },	  { BCKEND, "BCKEND" }, { MENU, "MENU" },
	{ MAIN, "MAIN" }, { STATE, "STATE" },	{ OTHER, "" },
};

char *get_module_name(enum module module)
{
	uint32_t i = 0;
	const struct Module *curr = &modules[i];
	while (curr->id != OTHER) {
		if (curr->id == module)
			return curr->name;
		curr = &modules[++i];
	}
	/* curr->id is OTHER */
	return curr->name;
}
