// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-view-presentation.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2010 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <gtk/gtk.h>

#include <papers-document.h>

G_BEGIN_DECLS

#define PPS_TYPE_VIEW_PRESENTATION (pps_view_presentation_get_type ())
PPS_PUBLIC
G_DECLARE_DERIVABLE_TYPE (PpsViewPresentation, pps_view_presentation, PPS, VIEW_PRESENTATION, GtkWidget)

PPS_PUBLIC
PpsViewPresentation *pps_view_presentation_new (PpsDocument *document,
                                                guint current_page,
                                                guint rotation,
                                                gboolean inverted_colors);
PPS_PUBLIC
guint pps_view_presentation_get_current_page (PpsViewPresentation *pview);
PPS_PUBLIC
void pps_view_presentation_next_page (PpsViewPresentation *pview);
PPS_PUBLIC
void pps_view_presentation_previous_page (PpsViewPresentation *pview);
PPS_PUBLIC
void pps_view_presentation_set_rotation (PpsViewPresentation *pview,
                                         gint rotation);
PPS_PUBLIC
guint pps_view_presentation_get_rotation (PpsViewPresentation *pview);

G_END_DECLS
