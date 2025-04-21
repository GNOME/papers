// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include <glib.h>

G_BEGIN_DECLS

typedef struct _PpsBackendInfo PpsBackendInfo;

struct _PpsBackendInfo {
	gchar *type_desc;
	gchar **mime_types;

	gchar *module_name;
	gboolean resident;
};

void _pps_backend_info_free (PpsBackendInfo *info);

GList *_pps_backend_info_load_from_dir (const char *path);

G_END_DECLS
