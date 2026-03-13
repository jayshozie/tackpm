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
	db_id id;			 /* id of the entity */
};

/* generalized selector struct */
struct sel_ctx {
	struct ncselector *widget; /* the notcurses widget */
	struct nav_node *map;	   /* parallel array of nav_nodes */
	db_count count;			   /* how many items are currently in the list? */
	char **opts;			   /* what will be written to the screen */
};

static struct sel_ctx *build_nav_pane(struct ncplane *np, struct tack_state *s)
{
	struct sel_ctx *sel;
	struct ncselector_item *items;
	db_count curr_idx = 0;

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
		asprintf(&sel->opts[curr_idx], "%s", s->c.cate_names[c]);
		items[curr_idx].option = sel->opts[curr_idx];
		items[curr_idx].desc = NULL;
		curr_idx++;

		for (proj_count p = 0; p < s->p.proj_count; p++) {
			if (s->p.proj_cids[p] == curr_cid) {
				sel->map[curr_idx] = (struct nav_node){
					.type = PROJECTS,
					.id = s->p.pids[p],
				};
				asprintf(&sel->opts[curr_idx], " %s", s->p.proj_names[p]);
				items[curr_idx].option = sel->opts[curr_idx];
				items[curr_idx].desc = NULL;
				curr_idx++;
			}
		}
	}
	items[curr_idx].option = NULL;
	items[curr_idx].desc = NULL;

	struct ncselector_options opts = {
		.title = NULL,
		.secondary = NULL,
		.footer = NULL,
		.items = items,
	};
	sel->widget = ncselector_create(np, &opts);
	free(items);
	return sel;
}

static inline bool task_matches(struct tack_state *s,
								db_tid target_tid,
								struct nav_node *filter)
{
	if (!filter)
		return false;
	if (filter->type == PROJECTS) {
		return (s->t.task_pids[target_tid] == filter->id);
	} else if (filter->type == CATEGORIES) {
		for (db_count i = 0; i < s->p.proj_count; i++) {
			if (s->p.pids[i] == s->t.task_pids[target_tid])
				return (s->p.proj_cids[i] == filter->id);
		}
	} else {
		log_error(
			MENU,
			"Something went wrong in task_matches (possibly called from build_task_pane). filter->id : %d | filter->type : %d",
			filter->id, filter->type);
	}
	return false;
}

static struct sel_ctx *build_task_pane(struct ncplane *np,
									   struct tack_state *s,
									   struct nav_node *filter)
{
	struct sel_ctx *sel;
	struct ncselector_item *items;
	db_count n_tasks, curr_idx = 0;
	char *done_status;

	if (!np || !s || !filter)
		return NULL;

	/* count first */
	n_tasks = 0;
	for (db_count i = 0; i < s->t.task_count; i++) {
		if (task_matches(s, i, filter))
			n_tasks++;
	}
	sel = malloc(sizeof(struct sel_ctx));
	if ((sel->count = n_tasks) == 0) {
		free(sel);
		return NULL;
	}
	sel->map = malloc(sel->count * sizeof(struct nav_node));
	sel->opts = malloc(sel->count * sizeof(char *));
	items = malloc((sel->count + 1) * sizeof(struct ncselector_item));
	for (db_count i = 0; i < s->t.task_count; i++) {
		done_status = "[ ]";
		if (task_matches(s, i, filter)) {
			sel->map[curr_idx] = (struct nav_node){
				.type = TASKS,
				.id = s->t.tids[i],
			};
			if (s->t.task_dones[i] == true)
				done_status = "[x]";
			asprintf(&sel->opts[curr_idx], "%s %s", done_status,
					 s->t.task_descs[i]);
			items[curr_idx].option = sel->opts[curr_idx];
			items[curr_idx].desc = NULL;
			curr_idx++;
		}
	}
	items[curr_idx].option = NULL;
	items[curr_idx].desc = NULL;

	struct ncselector_options opts = {
		.title = NULL,
		.secondary = NULL,
		.footer = NULL,
		.items = items,
	};
	sel->widget = ncselector_create(np, &opts);
	free(items);
	return sel;
}

static void destroy_sel(struct sel_ctx *sel)
{
	if (sel) {
		ncselector_destroy(sel->widget, NULL);
		free(sel->map);
		for (db_count i = 0; i < sel->count; i++) {
			free(sel->opts[i]);
		}
		free(sel->opts);
		free(sel);
	}
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
	struct ncplane *sidebar, *main, *std;
	struct nav_node filter;
	bool need_rebuild = true;
	ncplane_options sidebar_opts = { 0 }, main_opts = { 0 };
	struct sel_ctx *nav_sel = NULL, *task_sel = NULL;
	enum ui_focus focus = PANE_LEFT;
	enum state prev_state = STATE_HYDR;
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

		if (prev_state == STATE_HYDR && local_state == STATE_IDLE) {
			need_rebuild = true;
		}
		prev_state = local_state;

		if (!is_up[1]) {
			err = "DATABASE OFFLINE - PRESS 'q' TO QUIT";
			if (std) {
				ncplane_dim_yx(std, &y, &x);
				ncplane_erase(std);
				ncplane_printf_yx(std, y / 2, (x - strlen(err)) / 2, "%s", err);
			}
		} else if (local_state == STATE_HYDR) {
			err = "LOADING DATABASE...";
			if (std) {
				ncplane_dim_yx(std, &y, &x);
				ncplane_erase(std);
				ncplane_printf_yx(std, y / 2, (x - strlen(err)) / 2, "%s", err);
			}
		} else if (is_up[1]) {
			/* @TODO2: */
			if (std)
				ncplane_erase(std);
			if (sidebar)
				ncplane_erase(sidebar);
			if (main)
				ncplane_erase(main);

			if (need_rebuild) {
				pthread_mutex_lock(&s->lock);
				if (!nav_sel) {
					nav_sel = build_nav_pane(sidebar, s);
				}
				destroy_sel(task_sel);
				task_sel = NULL;
				if (nav_sel && nav_sel->count > 0) {
					const char *sel_str = ncselector_selected(nav_sel->widget);
					int idx = 0;
					for (int i = 0; i < nav_sel->count; i++) {
						if (strcmp(sel_str, nav_sel->opts[i]) == 0) {
							idx = i;
							break;
						}
					}
					filter = nav_sel->map[idx];
					task_sel = build_task_pane(main, s, &filter);
				}
				need_rebuild = false;
				pthread_mutex_unlock(&s->lock);
			}

			if (sidebar)
				ncplane_perimeter_rounded(sidebar, 0, 0, 0);
			if (main)
				ncplane_perimeter_rounded(main, 0, 0, 0);
		}

		notcurses_render(nc);

		struct timespec *ts = (local_state == STATE_IDLE) ? NULL : &timer;
		uint32_t key = notcurses_get(nc, ts, &ni);

		if (key == 'q') {
			pthread_mutex_lock(&s->lock);
			s->is_running = false;
			pthread_cond_signal(&s->cond);
			pthread_mutex_unlock(&s->lock);
			break;
		}

		if (local_state == STATE_IDLE && is_up[1]) {
			switch (key) {
			case 'j':
			case NCKEY_DOWN:
				if (focus == PANE_LEFT && nav_sel) {
					ncselector_nextitem(nav_sel->widget);
					need_rebuild = true;
				} else if (focus == PANE_RIGHT && task_sel) {
					ncselector_nextitem(task_sel->widget);
				}
				break;
			case 'k':
			case NCKEY_UP:
				if (focus == PANE_LEFT && nav_sel) {
					ncselector_previtem(nav_sel->widget);
					need_rebuild = true;
				} else if (focus == PANE_RIGHT && task_sel) {
					ncselector_previtem(task_sel->widget);
				}
				break;
			case 'l':
			case NCKEY_RIGHT:
			case NCKEY_ENTER:
			case KEY_ENTER_NL:
			case KEY_ENTER_CR:
				if (focus == PANE_LEFT)
					focus = PANE_RIGHT;
				break;
			case 'h':
			case NCKEY_LEFT:
			case NCKEY_ESC:
				if (focus == PANE_RIGHT)
					focus = PANE_LEFT;
				break;
			}
		}
	}
}
