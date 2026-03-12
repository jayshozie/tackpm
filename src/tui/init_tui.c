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

#include <tui/tui.h>

static inline void init_options(struct notcurses_options *opts)
{
    opts->flags = NCOPTION_SUPPRESS_BANNERS;
    opts->loglevel = NCLOGLEVEL_SILENT;
}

struct notcurses *init_tui(void)
{
    struct notcurses *nc;
    struct notcurses_options opts = {0};
    init_options(&opts);

    nc = notcurses_init(&opts, NULL);
    if (nc == NULL)
        return NULL;

    return nc;
}
