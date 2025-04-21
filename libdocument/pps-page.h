// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_PAGE (pps_page_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsPage, pps_page, PPS, PAGE, GObject)

typedef gpointer PpsBackendPage;
typedef GDestroyNotify PpsBackendPageDestroyFunc;

struct _PpsPage {
	GObject base_instance;

	gint index;

	PpsBackendPage backend_page;
	PpsBackendPageDestroyFunc backend_destroy_func;
};

PPS_PUBLIC
PpsPage *pps_page_new (gint index);

G_END_DECLS
