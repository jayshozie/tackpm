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

#ifndef _TUI_H
#define _TUI_H
#include <notcurses/notcurses.h>
#include <state/state.h>

enum ui_focus {
	PANE_LEFT,
	PANE_RIGHT,
	MODAL_INSERT,
	MODAL_DELETE,
};

typedef uint8_t ui_col;
typedef uint64_t ui_selected;

extern struct notcurses *init_tui(void);

extern void tui(struct notcurses *nc, struct tack_state *s);

#endif /* _TUI_H */
