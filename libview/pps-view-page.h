// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-view-page.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Markus Göllnitz <camelcasenick@bewares.it>
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

PpsViewPage *pps_view_page_new (void);

void pps_view_page_setup (PpsViewPage *page,
                          PpsDocumentModel *model,
                          PpsAnnotationsContext *annots_context,
                          PpsSearchContext *search_context,
                          PpsPageCache *page_cache,
                          PpsPixbufCache *pixbuf_cache);

void pps_view_page_set_page (PpsViewPage *page, gint index);
gint pps_view_page_get_page (PpsViewPage *page);

G_END_DECLS
