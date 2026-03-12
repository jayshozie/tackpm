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

#ifndef _STATE_H
#define _STATE_H
#include <stdbool.h>
#include <libpq-fe.h>
#include <db-defs.h>
#include <pthread.h>

enum state {
	STATE_IDLE, /* set by db_worker to idle the state, db_worker halts */
	STATE_HYDR, /* set by ui to fetch all tables, also the startup state */
	STATE_WRTE, /* set by ui to write to db state */
};

enum db_action {
    NONE,
	INSERT,
	DELETE,
	ALTER,
	TOGGLE,
};

enum db_entity {
    UNSET,
	CATEGORIES,
	PROJECTS,
	TASKS,
};

/* categories */
struct categories {
	cate_count cate_count; /* amount of categories */
	db_cid *cids;		   /* array holding category ids (cid) */
	char **cate_names;	   /* array holding the category names */
};

/* projects */
struct projects {
	proj_count proj_count; /* amount of projects */
	db_pid *pids;		   /* array holding project ids (pid) */
	db_cid *proj_cids;	   /* which project is in which category? */
	char **proj_names;	   /* array holding the category names */
};

/* languages */
struct languages {
	lang_count lang_count; /* amount of languages defined */
	db_lid *lids;		   /* array holding languages ids (lid) */
	char **lang_names;	   /* array holding the language names */
};

/* project-language pairs */
struct pl_pairs {
	pl_count projlong_count; /* amount of project-language pairs */
	db_pid *pids;			 /* array holding project ids (pid) */
	db_lid *lids;			 /* array holding language ids (lid) */
};

/* tasks */
struct tasks {
	task_count task_count; /* amount of tasks (total) */
	db_tid *tids;		   /* array holding all tasks */
	char **task_descs;	   /* array holding arrays of task descriptions */
	bool *task_dones;	   /* array holding bool dones of tasks */
	db_pid *task_pids;	   /* which task is in which project? */
};

struct task_params {
	char *desc; /* new description of the task */
	bool done;	/* whether the task should be set to done or not */
	db_pid pid; /* project id of the project which the task belongs to */
};

struct project_params {
	char *name; /* project's new name */
	db_cid cid; /* which category this project is in? */

	/* overwrites the existing set, so it should be a complete. */
	char *langs; /* DESTRUCTIVE: languages that is used in the project */
};

struct category_params {
	char *name; /* new name of the category */
};

/* this should be set by the ui. backend will read this. */
union query_params {
	struct task_params task;
	struct project_params project;
	struct category_params category;
};

/* query to be sent to the database. set by the ui, read by the db_worker */
struct query {
	enum db_action action; /* the action to be performed */
	enum db_entity table;  /* affected table */
	db_id id; /* affected id, should be 0 (int) if a new member is added */
	union query_params params; /* parameters to be given to the database */
};

struct tack_state {
	pthread_mutex_t lock; /* the mutex lock */
	enum state state;	  /* communication line between ui and db */
	pthread_cond_t cond;  /* thread condition for db wait */
	pthread_t db_thread;  /* db thread */
	bool is_running;	  /* is our program running? */
	bool is_db_up;		  /* is the postgresql service up? */
	PGconn *conn;		  /* database connection */

	/* only populated when a query is about to be performed */
	struct query query; /* currrent query to be performed */

	struct categories c; /* current state of categories */
	struct projects p;	 /* current state of projects */
	struct languages l;	 /* current state of languages */
	struct pl_pairs pl;	 /* current state of project-language pairs */
	struct tasks t;		 /* current state of tasks */
};

extern int init_state(struct tack_state *state);

#endif /* _STATE_H */
