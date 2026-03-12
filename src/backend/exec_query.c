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
#include <string.h>
#include <backend/backend.h>
#include <log/log.h>
#include <state/state.h>

/*
 * Gets a comma-separated, null-terminated list of languages, returns an array
 * of them.
 */
static inline char **parse_lang_list(char *langs)
{
	char **tokens, buf[1024], c;
	uint64_t comma_count = 0, ref_idx = 0, buf_idx = 0, tok_idx = 0;

	/* get the number of comments in the original string */
	while (langs[ref_idx] != '\0')
		if (langs[ref_idx++] == ',')
			comma_count++;

	/* iterate over the characters in the original string */
	ref_idx = 0;
	tokens = malloc((comma_count + 1) * sizeof(char *));
	while (langs[ref_idx] != '\0') {
		c = langs[ref_idx];
		if (buf_idx == 0 && (c == ' ' || c == '\t')) {
			/* then, it's whitespace before the actual token starts, ignore */
			ref_idx++;
		} else if (c == ',') {
			/* flush the buffer */
			if (buf_idx > 0) {
				buf[buf_idx] = '\0';
				tokens[tok_idx] = strdup(buf);
				tok_idx++;
				buf_idx = 0;
			}
			ref_idx++;
		} else {
			/* regular characters */
			buf[buf_idx] = c;
			buf_idx++;
			ref_idx++;
		}
	}
	if (buf_idx > 0) {
		buf[buf_idx] = '\0';
		tokens[tok_idx] = strdup(buf);
		tok_idx++;
		buf_idx = 0;
	}
	tokens[tok_idx] = NULL;

	return tokens;
}

static inline void execute_lang_query(PGconn *conn,
									  struct query *q,
									  const char *pid_str)
{
	char lid_str[32], **langs;
	const char *params[3], *lang_q, *pl_q;
	PGresult *res;

	langs = parse_lang_list(q->params.project.langs);
	if (!langs)
		return;

	lang_q =
		"INSERT INTO language (name) VALUES ($1) ON CONFLICT (name) DO UPDATE SET name=EXCLUDED.name RETURNING lid;";
	pl_q =
		"INSERT INTO project_language (project_id, language_id) VALUES ($1, $2);";

	/* iterate over languages */
	for (lang_count i = 0; langs[i] != NULL; i++) {
		params[0] = langs[i];
		res = PQexecParams(conn, lang_q, 1, NULL, params, NULL, NULL, 0);
		snprintf(lid_str, sizeof(lid_str), "%s", PQgetvalue(res, 0, 0));
		PQclear(res);
		params[0] = pid_str;
		params[1] = lid_str;
		res = PQexecParams(conn, pl_q, 2, NULL, params, NULL, NULL, 0);
		PQclear(res);
	}

	for (lang_count i = 0; langs[i] != NULL; i++) {
		free(langs[i]);
	}
	free(langs);
}

/*
 * Gets a query struct and the database connection, and executes the query.
 */
static inline void execute_category_query(PGconn *conn, struct query *q)
{
	PGresult *res;
	uint32_t n_params;
	char cid_str[32];
	const char *params[2];
	const char *query;
	if (q->id != 0)
		snprintf(cid_str, sizeof(cid_str), "%ld", q->id);
	switch (q->action) {
	case (INSERT):
		query = "INSERT INTO category (name) VALUES ($1);";
		params[0] = q->params.category.name;
		n_params = 1;
		break;
	case (DELETE):
		query = "DELETE FROM category WHERE cid = $1;";
		params[0] = cid_str;
		n_params = 1;
		break;
	case (ALTER):
		query = "UPDATE category SET name = $1 WHERE cid = $2;";
		params[0] = q->params.category.name;
		params[1] = cid_str;
		n_params = 2;
		break;
	default:
		n_params = 0;
		query = "BEGIN; END;";
		break;
	}
	res = PQexecParams(conn, query, n_params, NULL, params, NULL, NULL, 0);
	PQclear(res);
	if (q->params.category.name)
		free(q->params.category.name);
}

/*
 * Gets a query struct and the database connection, and executes the query.
 */
static inline void execute_project_query(PGconn *conn, struct query *q)
{
	query_status status = SUCCESS;
	PGresult *res;
	char pid_str[32], cid_str[32];
	uint32_t n_params;
	const char *params[3];
	const char *query;

	if (q->id != 0)
		snprintf(pid_str, sizeof(pid_str), "%ld", q->id);
	if (q->params.project.cid != 0)
		snprintf(cid_str, sizeof(cid_str), "%ld", q->params.project.cid);

	switch (q->action) {
	case (INSERT):
		query =
			"INSERT INTO project (name, category_id) VALUES ($1, $2) RETURNING pid;";
		params[0] = q->params.project.name;
		params[1] = cid_str;
		n_params = 2;
		PQexec(conn, "BEGIN;");
		res = PQexecParams(conn, query, n_params, NULL, params, NULL, NULL, 0);
		snprintf(pid_str, sizeof(pid_str), "%s", PQgetvalue(res, 0, 0));
		PQclear(res);
		execute_lang_query(conn, q, pid_str);
		PQexec(conn, "COMMIT;");
		break;
	case (DELETE):
		query = "DELETE FROM project WHERE pid = $1;";
		params[0] = pid_str;
		res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
		PQclear(res);
		break;
	case (ALTER):
		query =
			"UPDATE project SET name = $1, category_id = $2 WHERE pid = $3;";
		params[0] = q->params.project.name;
		params[1] = cid_str;
		params[2] = pid_str;
		n_params = 3;
		PQexec(conn, "BEGIN;");
		res = PQexecParams(conn, query, n_params, NULL, params, NULL, NULL, 0);
		PQclear(res);
		/* remove the old languages associated with this project */
		params[0] = pid_str;
		query = "DELETE FROM project_language WHERE project_id = $1";
		res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
		PQclear(res);
		execute_lang_query(conn, q, pid_str);
		PQexec(conn, "COMMIT;");
		break;
	default:
		/* error, it shouldn't be none or toggle in table project */
		break;
	}
	if (q->params.project.name)
		free(q->params.project.name);
	if (q->params.project.langs)
		free(q->params.project.langs);
}

static inline void execute_task_query(PGconn *conn, struct query *q)
{
	PGresult *res;
	char id_str[32], pid_str[32];
	const char *params[2];
	const char *query;
	const char *done_str;
	uint32_t n_params;

	if (q->id != 0)
		snprintf(id_str, sizeof(id_str), "%ld", q->id);
	if (q->params.task.pid != 0)
		snprintf(pid_str, sizeof(pid_str), "%ld", q->params.task.pid);

	switch (q->action) {
	case (INSERT):
		params[0] = q->params.task.desc;
		params[1] = pid_str;
		query = "INSERT INTO task (description, project_id) VALUES ($1, $2);";
		n_params = 2;
		break;
	case (DELETE):
		params[0] = id_str;
		query = "DELETE FROM task WHERE tid = $1;";
		n_params = 1;
		break;
	case (ALTER):
		params[0] = q->params.task.desc;
		params[1] = id_str;
		query = "UPDATE task SET description = $1 WHERE tid = $2;";
		n_params = 2;
		break;
	case (TOGGLE):
		query = "UPDATE task SET done = $1 WHERE tid = $2;";
		done_str = (q->params.task.done) ? "true" : "false";
		params[0] = done_str;
		params[1] = id_str;
		n_params = 2;
		break;
	default:
		/* error, it shouldn't be none */
		n_params = 0;
		query = "BEGIN; END;";
		break;
	}
	res = PQexecParams(conn, query, n_params, NULL, params, NULL, NULL, 0);
	PQclear(res);
	if (q->params.task.desc)
		free(q->params.task.desc);
}

/* @TODO: Implement
 */
query_status exec_query(PGconn *conn, struct tack_state *s)
{
	query_status status = SUCCESS;
	struct query q;
	/*
     * 1. lock the mutex
     * 2. declare local stack variable
     * 3. set state variables to null
     * 4. unlock the mutex
     * 5. call PQexecParams with the correct arguments
     * 6. free local variables
     * 7. lock the mutex
     * 8. set state to STATE_HYDR
     * 9. unlock mutex
     */

	pthread_mutex_lock(&s->lock);

	q = s->query;
	s->query.table = UNSET;
	memset(&s->query.params, 0, sizeof(union query_params));
	pthread_mutex_unlock(&s->lock);
	if (q.table == CATEGORIES) {
		execute_category_query(conn, &q);
	} else if (q.table == PROJECTS) {
		execute_project_query(conn, &q);
	} else if (q.table == TASKS) {
		execute_task_query(conn, &q);
	} else {
		/* error */
		log_error(BCKEND, "Failed while parsing the table information: %s",
				  q.table);
	}

	pthread_mutex_lock(&s->lock);
	s->query.action = NONE;
	s->state = STATE_HYDR;
	pthread_mutex_unlock(&s->lock);
	return status;
}
