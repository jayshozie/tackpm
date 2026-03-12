/* tack - A CLI project manager written in C \
Copyright (C) 2026  Emir Baha Yıldırım \

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

/* clang-format off */
#ifndef _DB_DEFS_H
#  define _DB_DEFS_H
#  ifndef _DB_DATA_TYPES
#    define _DB_DATA_TYPES
#include <stdint.h>
/* Counts */
/* general count data type */
typedef uint32_t db_count;
/* category count data type */
typedef uint32_t cate_count;
/* project count data type */
typedef uint32_t proj_count;
/* language count data type */
typedef uint32_t lang_count;
/* task count data type */
typedef uint32_t task_count;
/* project language count data type */
typedef uint32_t pl_count;

/* IDs */
/* general id data type */
typedef int64_t db_id;
/* category id (cid) data type */
typedef int64_t db_cid;
/* project id (pid) data type */
typedef int64_t db_pid;
/* language id (lid) data type */
typedef int64_t db_lid;
/* task id (tid) data type */
typedef int64_t db_tid;
#  endif /* _DB_DATA_TYPES */

#  ifndef _DB_TABLES
#    define _DB_TABLES

#    ifndef TAB_CATEGORY
#      define TAB_CATEGORY "category"
#      ifndef CATE_CID_COL
#        define CATE_CID_COL "name"
#      endif
#      ifndef CATE_NAME_COL
#        define CATE_NAME_COL "cid"
#      endif
#    endif

#    ifndef TAB_PROJECT
#      define TAB_PROJECT "project"
#      ifndef PROJ_PID_COL
#        define PROJ_PID_COL "pid"
#      endif
#      ifndef PROJ_NAME_COL
#        define PROJ_NAME_COL "name"
#      endif
#      ifndef PROJ_CID_COL
#        define PROJ_CID_COL "category_id"
#      endif
#    endif

#    ifndef TAB_LANGUAGE
#      define TAB_LANGUAGE "language"
#      ifndef LANG_LID_COL
#        define LANG_LID_COL "lid"
#      endif
#      ifndef LANG_NAME_COL
#        define LANG_NAME_COL "name"
#      endif
#    endif

#    ifndef TAB_PROJECT_LANGUAGE
#      define TAB_PROJECT_LANGUAGE "project_language"
#      ifndef PL_PID_COL
#        define PL_PID_COL "pid"
#      endif
#      ifndef PL_LID_COL
#        define PL_LID_COL "lid"
#      endif
#    endif

#    ifndef TAB_TASK
#      define TAB_TASK "task"
#      ifndef TASK_TID_COL
#        define TASK_TID_COL "tid"
#      endif
#      ifndef TASK_DESC_COL
#        define TASK_DESC_COL "description"
#      endif
#      ifndef TASK_DONE_COL
#        define TASK_DONE_COL "done"
#      endif
#      ifndef TASK_PID_COL
#        define TASK_PID_COL "project_id"
#      endif
#    endif

#  endif /* _DB_TABLES */

#endif /* _DB_DEFS_H */
