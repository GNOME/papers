/* pps-search-context.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2015 Igalia S.L.
 * Copyright (C) 2024 Markus Göllnitz  <camelcasenick@bewares.it>
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

#include <libdocument/pps-macros.h>
#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-document-model.h"
#include "pps-metadata.h"
#include "pps-search-result.h"

G_BEGIN_DECLS

#define PPS_TYPE_SEARCH_CONTEXT (pps_search_context_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsSearchContext, pps_search_context, PPS, SEARCH_CONTEXT, GObject)

struct _PpsSearchContext {
	GObject parent_instance;
};

struct _PpsSearchContextClass {
	GObjectClass parent_class;
};

PPS_PUBLIC
PpsSearchContext *pps_search_context_new (PpsDocumentModel *model);
PPS_PUBLIC
const gchar *pps_search_context_get_search_term (PpsSearchContext *context);
PPS_PUBLIC
void pps_search_context_set_search_term (PpsSearchContext *context,
                                         const gchar *search_term);
PPS_PUBLIC
PpsFindOptions pps_search_context_get_options (PpsSearchContext *context);
PPS_PUBLIC
void pps_search_context_set_options (PpsSearchContext *context,
                                     PpsFindOptions options);
PPS_PUBLIC
GtkSingleSelection *pps_search_context_get_result_model (PpsSearchContext *context);
PPS_PUBLIC
GPtrArray *pps_search_context_get_results_on_page (PpsSearchContext *context,
                                                   guint page);
gboolean pps_search_context_has_results_on_page (PpsSearchContext *context,
                                                 guint page);
PPS_PUBLIC
void pps_search_context_activate (PpsSearchContext *context);
PPS_PUBLIC
void pps_search_context_release (PpsSearchContext *context);
PPS_PUBLIC
gboolean pps_search_context_get_active (PpsSearchContext *context);
PPS_PUBLIC
void pps_search_context_restart (PpsSearchContext *context);
PPS_PUBLIC
void pps_search_context_autoselect_result (PpsSearchContext *context,
                                           PpsSearchResult *result);

G_END_DECLS
