/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2004 Red Hat, Inc
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

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include "pps-document-model.h"
#include <gtk/gtk-a11y.h>

#define PPS_TYPE_VIEW_ACCESSIBLE (pps_view_accessible_get_type ())
#define PPS_VIEW_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_VIEW_ACCESSIBLE, PpsViewAccessible))
#define PPS_IS_VIEW_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_VIEW_ACCESSIBLE))

typedef struct _PpsViewAccessible PpsViewAccessible;
typedef struct _PpsViewAccessibleClass PpsViewAccessibleClass;
typedef struct _PpsViewAccessiblePrivate PpsViewAccessiblePrivate;

struct _PpsViewAccessible {
	GtkContainerAccessible parent;

	PpsViewAccessiblePrivate *priv;
};

struct _PpsViewAccessibleClass {
	GtkContainerAccessibleClass parent_class;
};

GType pps_view_accessible_get_type (void);
AtkObject *pps_view_accessible_new (GtkWidget *widget);
void pps_view_accessible_set_model (PpsViewAccessible *accessible,
                                    PpsDocumentModel *model);
gint pps_view_accessible_get_n_pages (PpsViewAccessible *accessible);
gint pps_view_accessible_get_relevant_page (PpsViewAccessible *accessible);
void pps_view_accessible_set_page_range (PpsViewAccessible *accessible,
                                         gint start,
                                         gint end);
void _transform_doc_rect_to_atk_rect (PpsViewAccessible *accessible,
                                      gint page,
                                      PpsRectangle *doc_rect,
                                      PpsRectangle *atk_rect,
                                      AtkCoordType coord_type);
gboolean pps_view_accessible_is_doc_rect_showing (PpsViewAccessible *accessible,
                                                  gint page,
                                                  PpsRectangle *doc_rect);
void pps_view_accessible_set_focused_element (PpsViewAccessible *accessible,
                                              PpsMapping *new_focus,
                                              gint new_focus_page);
void pps_view_accessible_update_element_state (PpsViewAccessible *accessible,
                                               PpsMapping *element,
                                               gint element_page);
