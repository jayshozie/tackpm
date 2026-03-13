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

#include <stdlib.h>
#include <state/state.h>
#include <log/log.h>
#include <backend/backend.h>
#include <string.h>

/* @TODO_GENERAL: ERROR HANDLING
 *    First of all, check out the definition of the ConnStatusType enum:
 *
 *    ```c
 *    typedef enum {
 *        CONNECTION_OK,
 *        CONNECTION_BAD,
 *        // Non-blocking mode only below here // HERE
 *        ...
 *    } ConnStatusType;
 *    ```
 *
 *    As you can see, if the connection is in blocking mode (which we use), then
 *    the ConnStatusType can ONLY be `CONNECTION_OK` or `CONNECTION_BAD`.
 *
 *    This simplifies the error handling by a big margin.
 */

static bool db_exists(PGconn *conn)
{
	const char *q = "SELECT 1 from pg_database WHERE datname = 'tack_db'";
	PGresult *res = PQexec(conn, q);
	int cols = PQntuples(res);
	PQclear(res);
	if (cols != 0)
		return true;
	else
		return false;
}

/* @TODO: Change return to int.
 *        We have to handle database creation errors, and since this function
 *        doesn't return anything, we have no idea.
 */
static inline void create_db(PGconn *conn)
{
	const char *q = "CREATE DATABASE tack_db";
	PQexec(conn, q);
}

/* Idempotent. No need for schema checks outside of this. */
static inline void create_schemas(PGconn *conn)
{
	PGresult *res;
	const uint8_t table_count = 5;
	const char *queries[table_count];
	queries[0] = "CREATE TABLE IF NOT EXISTS category (\
                  cid bigint primary key generated always as identity,\
                  name varchar(80) not null unique check (name <> ''));";
	queries[1] = "CREATE TABLE IF NOT EXISTS project (\
                  pid bigint primary key generated always as identity,\
                  name varchar(80) not null check (name <> ''),\
                  category_id bigint references category(cid)\
                  on delete cascade,\
                  unique (name, category_id));";
	queries[2] = "CREATE TABLE IF NOT EXISTS language (\
                  lid bigint primary key generated always as identity,\
                  name varchar(80) not null check (name <> ''),\
                  unique (name));";
	queries[3] = "CREATE TABLE IF NOT EXISTS project_language (\
                  project_id bigint not null references project(pid)\
                  on delete cascade,\
                  language_id bigint not null references language(lid)\
                  on delete cascade,\
                  primary key(project_id, language_id));";
	queries[4] = "CREATE TABLE IF NOT EXISTS task (\
                  tid bigint primary key generated always as identity,\
                  description text not null check (description <> ''),\
                  done bool default false not null,\
                  project_id bigint not null references project(pid)\
                  on delete cascade,\
                  unique (description, project_id));";
	/* create category table */
	for (uint8_t i = 0; i < table_count; i++) {
		res = PQexec(conn, queries[i]);
		PQclear(res);
	}
}

static inline bool is_connection_up(PGconn *conn, struct tack_state **s)
{
	bool is_up = true;
	if (PQstatus(conn) == CONNECTION_BAD) {
		is_up = false;
		pthread_mutex_lock(&(*s)->lock);
		(*s)->is_db_up = false;
		pthread_mutex_unlock(&(*s)->lock);
		PQfinish(conn);
	}
	return is_up;
}

/* Checks `str` against: (all of them are strings)
 *   "NULl", "true", "false", "TRUE", "FALSE", "on", "off", "1", "0"
 * and returns the result as a boolean.
 *
 * If `str` is NULL or "NULL" or neither of the above, then the function returns
 * false and logs an error.
 */
/* clang-format off */
static inline bool strtobool(char *str)
{
	bool ret;

	if (!str) {
		goto error;
	} else if (strcmp(str, "NULL")) {
		goto error_log;
	} else if (*str == 'T' || *str == 't' ||
			   strcmp(str, "on") || *str == '1') {
		ret = true;
	} else if (*str == 'F' || *str == 'f' ||
			   strcmp(str, "off") || *str == '0') {
		ret = false;
	} else {
		goto error_log;
	}
	return ret;

error_log:
	log_error(BCKEND, "Couldn't get task's state : [%s]", str);
error:
	return false;
}
/* clang-format on */

/* @TODO: Error handling. */
/* fetches all categories */
static inline query_status fetch_categories(PGconn *conn, struct categories *nc)
{
	char *errmsg;
	ExecStatusType execstatus;
	uint8_t cid_col, name_col;
	query_status status = SUCCESS;
	const char *q = "SELECT cid, name FROM category ORDER BY cid;";
	PGresult *res = PQexec(conn, q);

	if (!nc) {
		log_error(BCKEND, "[fetch_projects: Uninitialized argument: `nc`");
		status = FAILURE;
		return status;
	}

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_error(BCKEND, "[fetch_categories]: %s", PQerrorMessage(conn));
		PQclear(res);
		nc->cate_count = 0;
		nc->cids = NULL;
		nc->cate_names = NULL;
		status = FAILURE;
		return status;
	}

	nc->cate_count = PQntuples(res);
	nc->cids = malloc(nc->cate_count * sizeof(db_cid));
	nc->cate_names = malloc(nc->cate_count * sizeof(char *));

	cid_col = PQfnumber(res, CATE_CID_COL);
	name_col = PQfnumber(res, CATE_NAME_COL);
	for (cate_count i = 0; i < nc->cate_count; i++) {
		nc->cids[i] = (db_cid)strtoull(PQgetvalue(res, i, cid_col), NULL, 10);
		nc->cate_names[i] = strdup(PQgetvalue(res, i, name_col));
	}

	PQclear(res);
	return status;
}

/* @TODO: Error handling. */
/* fetch all projects */
static inline query_status fetch_projects(PGconn *conn, struct projects *np)
{
	char *errmsg;
	ExecStatusType execstatus;
	uint8_t pid_col, name_col, cid_col;
	query_status status = SUCCESS;
	const char *q = "SELECT pid, name, category_id FROM project ORDER BY pid;";
	PGresult *res = PQexec(conn, q);

	if (!np) {
		log_error(BCKEND, "[fetch_projects: Uninitialized argument: `np`");
		status = FAILURE;
		return status;
	}

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_error(BCKEND, "[fetch_projects]: %s", PQerrorMessage(conn));
		PQclear(res);
		np->proj_count = 0;
		np->pids = NULL;
		np->proj_cids = NULL;
		np->proj_names = NULL;
		status = FAILURE;
		return status;
	}

	np->proj_count = PQntuples(res);
	np->pids = malloc(np->proj_count * sizeof(db_pid));

	pid_col = PQfnumber(res, PROJ_PID_COL);
	name_col = PQfnumber(res, PROJ_NAME_COL);
	cid_col = PQfnumber(res, PROJ_CID_COL);
	for (proj_count i = 0; i < np->proj_count; i++) {
		np->pids[i] = (db_pid)strtoull(PQgetvalue(res, i, pid_col), NULL, 10);
		np->proj_names[i] = strdup(PQgetvalue(res, i, name_col));
		np->proj_cids[i] = strtoull(PQgetvalue(res, i, cid_col), NULL, 10);
	}

	PQclear(res);
	return status;
}

/* @TODO: Error handling. */
/* fetch all languages */
static inline query_status fetch_languages(PGconn *conn, struct languages *nl)
{
	char *errmsg;
	ExecStatusType execstatus;
	uint8_t lid_col, name_col;
	query_status status = SUCCESS;
	const char *q = "SELECT lid, name FROM language ORDER BY lid;";
	PGresult *res = PQexec(conn, q);

	if (!nl) {
		log_error(BCKEND, "[fetch_projects: Uninitialized argument: `nl`");
		status = FAILURE;
		return status;
	}

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_error(BCKEND, "[fetch_projects]: %s", PQerrorMessage(conn));
		PQclear(res);
		nl->lang_count = 0;
		nl->lang_names = NULL;
		nl->lids = NULL;
		status = FAILURE;
		return status;
	}

	nl->lang_count = PQntuples(res);
	nl->lids = malloc(nl->lang_count * sizeof(db_pid));

	lid_col = PQfnumber(res, LANG_LID_COL);
	name_col = PQfnumber(res, LANG_NAME_COL);
	for (proj_count i = 0; i < (*nl).lang_count; i++) {
		nl->lids[i] = (db_pid)strtoull(PQgetvalue(res, i, lid_col), NULL, 10);
		nl->lang_names[i] = strdup(PQgetvalue(res, i, name_col));
	}

	PQclear(res);
	return status;
}

/* @TODO: Error handling. */
/* fetch all project-language pairs */
static inline query_status fetch_pl_pairs(PGconn *conn, struct pl_pairs *np)

{
	char *errmsg;
	ExecStatusType execstatus;
	uint8_t pid_col, lid_col;
	query_status status = SUCCESS;
	const char *q =
		"SELECT project_id, language_id FROM project_language ORDER BY project_id;";
	PGresult *res = PQexec(conn, q);

	if (!np) {
		log_error(BCKEND, "[fetch_projects: Uninitialized argument: `np`");
		status = FAILURE;
		return status;
	}

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_error(BCKEND, "[fetch_projects]: %s", PQerrorMessage(conn));
		PQclear(res);
		np->projlong_count = 0;
		np->lids = NULL;
		np->pids = NULL;
		status = FAILURE;
		return status;
	}

	np->projlong_count = PQntuples(res);

	pid_col = PQfnumber(res, PL_PID_COL);
	lid_col = PQfnumber(res, PL_LID_COL);
	for (pl_count i = 0; i < np->projlong_count; i++) {
		np->pids[i] = (db_pid)strtoull(PQgetvalue(res, i, pid_col), NULL, 10);
		np->lids[i] = (db_lid)strtoull(PQgetvalue(res, i, lid_col), NULL, 10);
	}

	PQclear(res);
	return status;
}

/* @TODO: Error handling. */
/* fetch all tasks */
static inline query_status fetch_tasks(PGconn *conn, struct tasks *nt)
{
	char *errmsg;
	ExecStatusType execstatus;
	uint8_t tid_col, desc_col, done_col, pid_col;
	query_status status = SUCCESS;
	const char *q =
		"SELECT tid, description, done, project_id FROM task ORDER BY tid;";
	PGresult *res = PQexec(conn, q);

	if (!nt) {
		log_error(BCKEND, "[fetch_projects: Uninitialized argument: `nt`");
		status = FAILURE;
		return status;
	}

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_error(BCKEND, "[fetch_projects]: %s", PQerrorMessage(conn));
		PQclear(res);
		nt->task_count = 0;
		nt->task_descs = NULL;
		nt->task_dones = NULL;
		nt->task_pids = NULL;
		nt->tids = NULL;
		status = FAILURE;
		return status;
	}

	nt->task_count = PQntuples(res);

	tid_col = PQfnumber(res, TASK_TID_COL);
	desc_col = PQfnumber(res, TASK_DESC_COL);
	done_col = PQfnumber(res, TASK_DONE_COL);
	pid_col = PQfnumber(res, TASK_PID_COL);
	for (task_count i = 0; i < nt->task_count; i++) {
		nt->tids[i] = (db_tid)strtoull(PQgetvalue(res, i, tid_col), NULL, 10);
		nt->task_descs[i] = strdup(PQgetvalue(res, i, desc_col));
		nt->task_dones[i] = strtobool(PQgetvalue(res, i, done_col));
		nt->task_pids[i] =
			(db_pid)strtoull(PQgetvalue(res, i, pid_col), NULL, 10);
	}

	PQclear(res);
	return status;
}

static inline int cleanup_state(struct tack_state **s)
{
	int status = 0;

	/* categories */
	if ((*s)->c.cids)
		free((*s)->c.cids);
	if ((*s)->c.cate_names) {
		for (cate_count i = 0; i < (*s)->c.cate_count; i++) {
			free((*s)->c.cate_names[i]);
		}
		free((*s)->c.cate_names);
	}

	/* projects */
	if ((*s)->p.pids)
		free((*s)->p.pids);
	if ((*s)->p.proj_cids)
		free((*s)->p.proj_cids);
	if ((*s)->p.proj_names) {
		for (proj_count i = 0; i < (*s)->p.proj_count; i++) {
			free((*s)->p.proj_names[i]);
		}
		free((*s)->p.proj_names);
	}

	/* languages */
	if ((*s)->l.lids)
		free((*s)->l.lids);
	if ((*s)->l.lang_names) {
		for (lang_count i = 0; i < (*s)->l.lang_count; i++) {
			free((*s)->l.lang_names[i]);
		}
		free((*s)->l.lang_names);
	}

	/* project-language pairs */
	if ((*s)->pl.pids)
		free((*s)->pl.pids);
	if ((*s)->pl.lids)
		free((*s)->pl.lids);

	/* tasks */
	if ((*s)->t.tids)
		free((*s)->t.tids);
	if ((*s)->t.task_descs) {
		for (task_count i = 0; i < (*s)->t.task_count; i++) {
			free((*s)->t.task_descs[i]);
		}
		free((*s)->t.task_descs);
	}
	if ((*s)->t.task_dones)
		free((*s)->t.task_dones);
	if ((*s)->t.task_pids)
		free((*s)->t.task_pids);

	return status;
}

/* @TODO: Error handling. */
/* fetch database completely */
static inline query_status fetch_db(PGconn *conn, struct tack_state *s)
{
	query_status status = SUCCESS;
	struct categories new_categories;
	struct projects new_projects;
	struct languages new_languages;
	struct pl_pairs new_pl_pairs;
	struct tasks new_tasks;
	/* @TODO START */
	fetch_categories(conn, &new_categories);
	fetch_projects(conn, &new_projects);
	fetch_languages(conn, &new_languages);
	fetch_pl_pairs(conn, &new_pl_pairs);
	fetch_tasks(conn, &new_tasks);
	/* @TODO END */

	pthread_mutex_lock(&s->lock);
	/* update state and free old temporary state arrays here (if not NULL) */

	/* @TODO returns status */
	/* free old data */
	cleanup_state(&s);

	/* categories */
	s->c = new_categories;
	/* projects */
	s->p = new_projects;
	/* languages */
	s->l = new_languages;
	/* project-language pairs */
	s->pl = new_pl_pairs;
	/* tasks */
	s->t = new_tasks;
	s->state = STATE_IDLE;
	pthread_mutex_unlock(&s->lock);

	return status;
}

/* @TODO1: Refactor first part until create_schemas.
 *         Write an inline function that establishes the first connection,
 *         does the checks, calls create_db if necessary, switches connections,
 *         then returns.
 * @TODO2: Implement the fetch cycle.
 *         It should also be written as an inline function.
 */
void *db_worker(void *arg)
{
	struct tack_state *s = (struct tack_state *)arg;
	PGconn *conn = PQconnectdb("dbname=postgres");

	/* @TODO1 */
	if (is_connection_up(conn, &s) == false)
		return NULL;

	if (db_exists(conn) == false)
		create_db(conn);

	/* switch connections */
	PQfinish(conn);
	conn = PQconnectdb("dbname=tack_db");

	if (is_connection_up(conn, &s) == false)
		return NULL;

	create_schemas(conn);

	pthread_mutex_lock(&s->lock);
	while (s->is_running) {
		/* magic wait, assume you're running normally after this line */
		while (s->state == STATE_IDLE && s->is_running) {
			pthread_cond_wait(&s->cond, &s->lock);
		}

		if (!s->is_running)
			break;

		/* @TODO2 */
		enum state cmd = s->state;
		pthread_mutex_unlock(&s->lock);
		switch (cmd) {
		case (STATE_HYDR):
			fetch_db(conn, s);
			break;
		case (STATE_WRTE):
			exec_query(conn, s);
			fetch_db(conn, s);
			break;
		case (STATE_IDLE):
			/* If we're down here */
		default:
			/* or here, something went terribly wrong */
			break;
		};

lock:
		pthread_mutex_lock(&s->lock);
		s->state = STATE_IDLE;
	}
	pthread_mutex_unlock(&s->lock);
	PQfinish(conn);
	return NULL;
}
