// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * pps-debug.c
 * This file is part of Papers
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Ppsan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002 - 2005 Paolo Maggi
 */

/*
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id: gedit-debug.c 4809 2006-04-08 14:46:31Z pborelli $
 */

/* Modified by Papers Team */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pps-debug.h"

static gboolean pps_debug_borders_initialized = FALSE;
static PpsDebugBorders pps_debug_borders = PPS_DEBUG_BORDER_NONE;

const GDebugKey border_keys[] = {
	{ "chars", PPS_DEBUG_BORDER_CHARS },
	{ "links", PPS_DEBUG_BORDER_LINKS },
	{ "forms", PPS_DEBUG_BORDER_FORMS },
	{ "annots", PPS_DEBUG_BORDER_ANNOTS },
	{ "images", PPS_DEBUG_BORDER_IMAGES },
	{ "media", PPS_DEBUG_BORDER_MEDIA },
	{ "selections", PPS_DEBUG_BORDER_SELECTIONS }
};

PpsDebugBorders
pps_debug_get_debug_borders (void)
{
	if (!pps_debug_borders_initialized) {
		pps_debug_borders = g_parse_debug_string (g_getenv ("PPS_DEBUG_SHOW_BORDERS"),
		                                          border_keys, G_N_ELEMENTS (border_keys));
		pps_debug_borders_initialized = TRUE;
	}
	return pps_debug_borders;
}
