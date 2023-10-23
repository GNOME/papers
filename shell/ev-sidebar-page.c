/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/*
 *  Copyright (C) 2005 Marco Pesenti Gritti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#include "ev-sidebar-page.h"

G_DEFINE_TYPE (EvSidebarPage, ev_sidebar_page, GTK_TYPE_BOX)

gboolean
ev_sidebar_page_support_document (EvSidebarPage *sidebar_page,
				  EvDocument    *document)
{
	EvSidebarPageClass *klass;

	g_return_val_if_fail (EV_IS_SIDEBAR_PAGE (sidebar_page), FALSE);
	g_return_val_if_fail (EV_IS_DOCUMENT (document), FALSE);

	klass = EV_SIDEBAR_PAGE_GET_CLASS (sidebar_page);

	g_return_val_if_fail (klass->support_document, FALSE);

	return klass->support_document (sidebar_page, document);
}

void
ev_sidebar_page_set_model (EvSidebarPage   *sidebar_page,
			   EvDocumentModel *model)
{
	EvSidebarPageClass *klass;

	g_return_if_fail (EV_IS_SIDEBAR_PAGE (sidebar_page));
	g_return_if_fail (EV_IS_DOCUMENT_MODEL (model));

	klass = EV_SIDEBAR_PAGE_GET_CLASS (sidebar_page);

	g_assert (klass->set_model);

	klass->set_model (sidebar_page, model);
}

const gchar *
ev_sidebar_page_get_label (EvSidebarPage *sidebar_page)
{
	EvSidebarPageClass *klass;

	g_return_val_if_fail (EV_IS_SIDEBAR_PAGE (sidebar_page), NULL);

	klass = EV_SIDEBAR_PAGE_GET_CLASS (sidebar_page);

	g_assert (klass->get_label);

	return klass->get_label (sidebar_page);
}

static void
ev_sidebar_page_class_init (EvSidebarPageClass *klass)
{

}

static void
ev_sidebar_page_init (EvSidebarPage *sidebar_page)
{
	gtk_orientable_set_orientation (GTK_ORIENTABLE (sidebar_page),
					GTK_ORIENTATION_VERTICAL);
}
