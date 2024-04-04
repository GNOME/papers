/*
 * pps-debug.h
 * This file is part of Papers
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Ppsan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002 - 2005 Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/*
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id: gedit-debug.h 4809 2006-04-08 14:46:31Z pborelli $
 */

/* Modified by Papers Team */

#pragma once

#if !defined (PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include <glib-object.h>

#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_GET_TYPE_NAME(instance) g_type_name_from_instance ((gpointer)instance)

#ifndef PPS_ENABLE_DEBUG

#define _pps_debug_init()
#define _pps_debug_shutdown()
#define pps_profiler_start(...) G_STMT_START { } G_STMT_END
#define pps_profiler_stop(...) G_STMT_START { } G_STMT_END

#else /* ENABLE_DEBUG */

typedef enum {
        PPS_DEBUG_BORDER_NONE       = 0,
        PPS_DEBUG_BORDER_CHARS      = 1 << 0,
        PPS_DEBUG_BORDER_LINKS      = 1 << 1,
        PPS_DEBUG_BORDER_FORMS      = 1 << 2,
        PPS_DEBUG_BORDER_ANNOTS     = 1 << 3,
        PPS_DEBUG_BORDER_IMAGES     = 1 << 4,
        PPS_DEBUG_BORDER_MEDIA      = 1 << 5,
        PPS_DEBUG_BORDER_SELECTIONS = 1 << 6,
        PPS_DEBUG_BORDER_ALL        = (1 << 7) - 1
} PpsDebugBorders;

void _pps_debug_init     (void);

PPS_PRIVATE
PpsDebugBorders pps_debug_get_debug_borders (void);

#endif /* PPS_ENABLE_DEBUG */

#ifdef HAVE_SYSPROF

#include <sysprof-capture.h>

#define PPS_PROFILER_START(job_type, message)                 \
	int64_t sysprof_begin = SYSPROF_CAPTURE_CURRENT_TIME; \
	const char* sysprof_name = job_type;                  \
	g_autofree const char* sysprof_message = message;
#define PPS_PROFILER_STOP() \
	sysprof_collector_mark(sysprof_begin,                                \
			       SYSPROF_CAPTURE_CURRENT_TIME - sysprof_begin, \
			       "papers",                                     \
			       sysprof_name,                                 \
			       sysprof_message);

#else /* HAVE_SYSPROF */

#define PPS_PROFILER_START(job_type)
#define PPS_PROFILER_STOP()

#endif /* HAVE_SYSPROF */

G_END_DECLS
