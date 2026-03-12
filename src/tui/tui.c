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

#include <stdint.h>
#include <time.h>
#include <string.h>
#include <log/log.h>
#include <tui/tui.h>

struct nav_node {
	enum db_entity type; /* CATEGORIES, PROJECTS, or TASKS */
	db_id id;
};

/* generalized selector struct */
struct sel_ctx {
	struct ncselector *widget; /* the notcurses widget */
	struct nav_node *map;	   /* parallel array of nav_nodes */
	db_count count;			   /* how many items are currently in the list? */
	char **opts;
};

static struct sel_ctx *build_nav_pane(struct ncplane *np, struct tack_state *s)
{
	struct sel_ctx *sel;
	struct ncselector_item *items;
	db_count total_nodes, curr_idx = 0;

	if (!np || !s)
		return NULL;

	sel = malloc(sizeof(struct sel_ctx));
	if ((sel->count = s->c.cate_count + s->p.proj_count) == 0) {
		free(sel);
		return NULL;
	}
	sel->map = malloc(sel->count * sizeof(struct nav_node));
	sel->opts = malloc(sel->count * sizeof(char *));
	items = malloc((sel->count + 1) * sizeof(struct ncselector_item));
	for (cate_count c = 0; c < s->c.cate_count; c++) {
		db_cid curr_cid = s->c.cids[c];
		sel->map[curr_idx] = (struct nav_node){
			.type = CATEGORIES,
			.id = curr_cid,
		};
		asprintf(&sel->opts[c], "%s", s->c.cate_names[c]);
		items[curr_idx].option = sel->opts[curr_idx];
		items[curr_idx].desc = NULL;
		curr_idx++;

		for (proj_count p = 0; p < s->p.proj_count; p++) {
			if (s->p.proj_cids[p] == curr_cid) {
				sel->map[curr_idx] = (struct nav_node){
					.type = PROJECTS,
					.id = s->p.pids[p],
				};
				asprintf(&sel->opts[p], " %s", s->p.proj_names[p]);
				items[curr_idx].option = sel->opts[curr_idx];
				items[curr_idx].desc = NULL;
				curr_idx++;
			}
		}
	}
	items[curr_idx].desc = NULL;
	items[curr_idx].option = NULL;

	struct ncselector_options opts = {
		.title = NULL,
		.secondary = NULL,
		.footer = NULL,
		.items = items,
	};
	sel->widget = ncselector_create(np, &opts);
	return sel;
}

static struct sel_ctx *build_task_pane(struct ncplane *np,
									   struct tack_state *s,
									   struct nav_node *filter)
{
	if (!np || !s || !filter)
		return NULL;

	if (filter->type == CATEGORIES) {
		for (db_count t = 0; t < s->t.task_count; t++) {
			if (s->t.task_pids[t] == filter->id) {
			}
		}
	} else if (filter->type == PROJECTS) {
	} else {
		/* something went wrong */
		log_error(MENU,
				  "Something went wrong in build_task_pane. filter.type : %d",
				  filter->type);
	}
}

static void destroy_sel(struct sel_ctx *sel)
{
	if (sel) {
		ncselector_destroy(sel->widget, NULL);
		free(sel->map);
		free(sel);
	}
	for (db_count i = 0; i < sel->count; i++) {
		free(sel->opts[i]);
	}
	free(sel->opts);
}

static inline void set_planes(struct notcurses *nc,
							  struct ncplane **std,
							  struct ncplane **sidebar,
							  struct ncplane **main,
							  ncplane_options *sidebar_opts,
							  ncplane_options *main_opts)
{
	*std = notcurses_stdplane(nc);

	sidebar_opts->y = 0, sidebar_opts->x = 0;
	sidebar_opts->cols = 30;
	sidebar_opts->flags = NCPLANE_OPTION_MARGINALIZED;
	sidebar_opts->margin_b = 0;

	main_opts->y = 0, main_opts->x = 30;
	main_opts->flags = NCPLANE_OPTION_MARGINALIZED;
	main_opts->margin_b = 0, main_opts->margin_r = 0;

	*sidebar = ncplane_create(*std, sidebar_opts);
	*main = ncplane_create(*std, main_opts);
}

/* @TODO1: Is this check even necessary? If it is:
 *         We're exiting. We should show a small message, and do a shutdown
 *         sequence while we're waiting.
 * @TODO2: Draw normal dashboard logic here.
 */
void tui(struct notcurses *nc, struct tack_state *s)
{
	uint32_t x, y;
	char *err;
	ncinput ni;
	bool is_up[2];
	ui_col cursor_y;
	db_cid active_cid;
	db_pid active_pid;
	db_tid active_tid;
	struct ncplane *sidebar, *main, *std;
	struct nav_node filter;
	bool need_rebuild = true;
	ncplane_options sidebar_opts = { 0 }, main_opts = { 0 };
	struct sel_ctx *nav_sel = NULL, *task_sel = NULL;
	enum ui_focus focus = PANE_LEFT;
	struct timespec timer = {
		.tv_sec = 0,
		.tv_nsec = 50000000, /* 50 milliseconds */
	};

	is_up[0] = true; /* references tack_state.is_running */
	is_up[1] = true; /* references tack_state.is_db_up */

	if (!nc)
		return;

	set_planes(nc, &std, &sidebar, &main, &sidebar_opts, &main_opts);
	pthread_mutex_lock(&s->lock);
	if (s->state == STATE_HYDR)
		pthread_cond_signal(&s->cond);
	pthread_mutex_unlock(&s->lock);

	while (is_up[0]) {
		pthread_mutex_lock(&s->lock);
		enum state local_state = s->state;
		is_up[0] = s->is_running;
		is_up[1] = s->is_db_up;
		pthread_mutex_unlock(&s->lock);

		if (!is_up[0]) {
			/* @TODO1 */
			break;
		}

		if (local_state == STATE_HYDR) {
			err = "LOADING DATABASE...";
			ncplane_dim_yx(std, &y, &x);
			ncplane_erase(std);
			ncplane_printf_yx(std, y / 2, (x - strlen(err)) / 2, "%s", err);
		} else if (is_up[1]) {
			/* @TODO2: */
			ncplane_erase(sidebar);
			ncplane_erase(main);

			if (need_rebuild) {
				pthread_mutex_lock(&s->lock);
				destroy_sel(nav_sel);
				destroy_sel(task_sel);
				nav_sel = build_nav_pane(sidebar, s);
				filter = nav_sel->map[0];
				task_sel = build_task_pane(main, s, &filter);

				need_rebuild = false;
				/*
                 * 1. Lock the thread.
                 * 2. 
                 * 3. Write to screen.
                 * 4. 
                 * 5. Call pthread_cond_signal(&s->cond); to get the db_worker out of
                 *    its locked-state.
                 * 6. Unlock the mutex.
                 */
				pthread_mutex_unlock(&s->lock);
			}

			ncplane_perimeter_rounded(sidebar, 0, 0, 0);
			ncplane_perimeter_rounded(main, 0, 0, 0);
		} else {
			err = "DATABASE OFFLINE - PRESS 'q' TO QUIT";
			ncplane_dim_yx(std, &y, &x);
			ncplane_erase(std);
			ncplane_printf_yx(std, y / 2, (x - strlen(err)) / 2, "%s", err);
		}

		notcurses_render(nc);

		struct timespec *ts = (local_state == STATE_IDLE) ? NULL : &timer;
		uint32_t key = notcurses_get(nc, ts, &ni);

		/*
         * we have to treat it separately. if we include it in the switch-case,
         * then the last break won't break out of the loop but the switch-case.
         */
		if (key == 'q') {
			pthread_mutex_lock(&s->lock);
			s->is_running = false;
			pthread_cond_signal(&s->cond);
			pthread_mutex_unlock(&s->lock);
			break;
		} else if (key != 0) {
			switch (key) {
			case (0):
				break;
			case ('j'):
				ni.id = NCKEY_DOWN;
				break;
			case ('k'):
				ni.id = NCKEY_UP;
				break;
			case ('\e'):
			case ('h'):
				ni.id = NCKEY_LEFT;
				break;
			case ('\r'):
			case ('\n'):
			case ('l'):
				ni.id = NCKEY_RIGHT;
				break;
			default:
				/* do nothing, we don't know the key, so don't touch it */
				break;
			}
			if (nav_sel && ncselector_offer_input(nav_sel->widget, &ni)) {
				const char *active = ncselector_selected(nav_sel->widget);
				for (db_count i = 0; i < nav_sel->count; i++) {
					if (strcmp(active, nav_sel->opts[i]) == 0) {
						pthread_mutex_lock(&s->lock);
						filter = nav_sel->map[i];
						destroy_sel(task_sel);
						task_sel = build_task_pane(main, s, filter);
						pthread_mutex_unlock(&s->lock);
					}
				}
			}
		}
	}
	notcurses_stop(nc);
}
