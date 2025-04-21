// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2004 Red Hat, Inc
 */

#include "pps-view-cursor.h"

static const gchar *cursors[] = {
	[PPS_VIEW_CURSOR_NORMAL] = NULL,
	[PPS_VIEW_CURSOR_IBEAM] = "text",
	[PPS_VIEW_CURSOR_LINK] = "pointer",
	[PPS_VIEW_CURSOR_WAIT] = "wait",
	[PPS_VIEW_CURSOR_HIDDEN] = "none",
	[PPS_VIEW_CURSOR_DRAG] = "grabbing",
	[PPS_VIEW_CURSOR_ADD] = "crosshair",
};

const gchar *
pps_view_cursor_name (PpsViewCursor cursor)
{
	if (cursor < G_N_ELEMENTS (cursors))
		return cursors[cursor];

	return NULL;
}
