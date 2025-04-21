// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#include <config.h>

#include "pps-page.h"

G_DEFINE_TYPE (PpsPage, pps_page, G_TYPE_OBJECT)

static void
pps_page_init (PpsPage *page)
{
}

static void
pps_page_finalize (GObject *object)
{
	PpsPage *page = PPS_PAGE (object);

	if (page->backend_destroy_func) {
		page->backend_destroy_func (page->backend_page);
		page->backend_destroy_func = NULL;
	}
	page->backend_page = NULL;

	(*G_OBJECT_CLASS (pps_page_parent_class)->finalize) (object);
}

static void
pps_page_class_init (PpsPageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = pps_page_finalize;
}

PpsPage *
pps_page_new (gint index)
{
	PpsPage *page;

	page = PPS_PAGE (g_object_new (PPS_TYPE_PAGE, NULL));
	page->index = index;

	return page;
}
