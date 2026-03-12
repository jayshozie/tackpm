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

#include <state.h>
#include <tui.h>
#include <log.h>
#include <backend.h>

// int main(int argc, char **argv)
int main(void)
{
	/* WIP: refactor in process */
	struct tack_state state;
	struct notcurses *nc;

	// will be implemented after global state is established
	// process_args(argc, &argv);

	init_state(&state);
	nc = init_tui();

	// print error to stderr and return
	if (!nc) {
		return -1; // placeholder
	}

	tui(nc, &state); // main loop is inside tui()

	pthread_join(state.db_thread, NULL);
	return 0;
}
