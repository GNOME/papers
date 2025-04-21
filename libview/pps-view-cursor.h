// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2004 Red Hat, Inc
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	PPS_VIEW_CURSOR_NORMAL,
	PPS_VIEW_CURSOR_IBEAM,
	PPS_VIEW_CURSOR_LINK,
	PPS_VIEW_CURSOR_WAIT,
	PPS_VIEW_CURSOR_HIDDEN,
	PPS_VIEW_CURSOR_DRAG,
	PPS_VIEW_CURSOR_ADD
} PpsViewCursor;

const gchar *pps_view_cursor_name (PpsViewCursor cursor);

G_END_DECLS
