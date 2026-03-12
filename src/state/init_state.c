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

#include <string.h>
#include <state.h>
#include <backend.h>

static inline void set_state(struct tack_state **s)
{
    memset((*s), 0, sizeof(struct tack_state));

    /* global */
	(*s)->is_running = true;
    // (*s)->is_db_up = false;
	(*s)->state = STATE_HYDR;
}

int init_state(struct tack_state *s)
{
	int status = 0;

    set_state(&s);

    status = pthread_cond_init(&s->cond, NULL);
	if (status != 0)
        return status;
    status = pthread_mutex_init(&s->lock, NULL);
	if (status != 0)
        return status;
    status = pthread_create(&s->db_thread, NULL, db_worker, s);
    return status;
}
