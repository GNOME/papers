/* pps-search-result.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Markus Göllnitz <camelcasenick@bewares.it>
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

G_BEGIN_DECLS

PPS_PUBLIC
#define PPS_TYPE_SEARCH_RESULT (pps_search_result_get_type ())

G_DECLARE_FINAL_TYPE (PpsSearchResult, pps_search_result, PPS, SEARCH_RESULT, GObject)

struct _PpsSearchResult {
	GObject parent_instance;
};

struct _PpsSearchResultClass {
	GObjectClass parent_class;
};

PpsSearchResult *pps_search_result_new (gchar *markup,
                                        gchar *label,
                                        guint page,
                                        guint index,
                                        guint global_index,
                                        PpsFindRectangle *rect);

PPS_PUBLIC
const gchar *pps_search_result_get_markup (PpsSearchResult *self);
PPS_PUBLIC
const gchar *pps_search_result_get_label (PpsSearchResult *self);
PPS_PUBLIC
guint pps_search_result_get_page (PpsSearchResult *self);
PPS_PUBLIC
guint pps_search_result_get_index (PpsSearchResult *self);
PPS_PUBLIC
guint pps_search_result_get_global_index (PpsSearchResult *self);
PPS_PUBLIC
GList *pps_search_result_get_rectangle_list (PpsSearchResult *self);
PPS_PUBLIC
void pps_search_result_append_rectangle (PpsSearchResult *self, PpsFindRectangle *rect);

G_END_DECLS
