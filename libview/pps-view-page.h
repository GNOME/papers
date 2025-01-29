/* pps-view-page.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Markus Göllnitz <camelcasenick@bewares.it>
 *
 * Papers is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Papers is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#pragma once

#include <papers-document.h>
#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <gtk/gtk.h>

#include "context/pps-document-model.h"
#include "context/pps-search-context.h"
#include "pps-page-cache.h"
#include "pps-pixbuf-cache.h"

G_BEGIN_DECLS

#define PPS_TYPE_VIEW_PAGE (pps_view_page_get_type ())

G_DECLARE_FINAL_TYPE (PpsViewPage, pps_view_page, PPS, VIEW_PAGE, GtkWidget)

struct _PpsViewPage {
	GtkWidget parent_instance;
};

struct _PpsViewPageClass {
	GtkWidgetClass parent_class;
};

PpsViewPage *pps_view_page_new (void);

void pps_view_page_setup (PpsViewPage *page,
                          PpsDocumentModel *model,
                          PpsSearchContext *search_context,
                          PpsPageCache *page_cache,
                          PpsPixbufCache *pixbuf_cache);

void pps_view_page_set_page (PpsViewPage *page, gint index);
gint pps_view_page_get_page (PpsViewPage *page);

G_END_DECLS
