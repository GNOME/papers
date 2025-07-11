/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2009 Carlos Garcia Campos
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

#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <glib-object.h>
#include <papers-document.h>

G_BEGIN_DECLS

PPS_PUBLIC
#define PPS_TYPE_DOCUMENT_MODEL (pps_document_model_get_type ())

G_DECLARE_FINAL_TYPE (PpsDocumentModel, pps_document_model, PPS, DOCUMENT_MODEL, GObject)

/**
 * PpsSizingMode:
 * @PPS_SIZING_FIT_PAGE:
 * @PPS_SIZING_FIT_WIDTH:
 * @PPS_SIZING_FREE:
 * @PPS_SIZING_AUTOMATIC:
 */
typedef enum {
	PPS_SIZING_FIT_PAGE,
	PPS_SIZING_FIT_WIDTH,
	PPS_SIZING_FREE,
	PPS_SIZING_AUTOMATIC
} PpsSizingMode;

typedef enum {
	PPS_PAGE_LAYOUT_SINGLE,
	PPS_PAGE_LAYOUT_DUAL,
	PPS_PAGE_LAYOUT_AUTOMATIC
} PpsPageLayout;

typedef enum {
	PPS_ANNOTATION_EDITING_STATE_NONE = 0,
	PPS_ANNOTATION_EDITING_STATE_INK = 1 << 0,
	PPS_ANNOTATION_EDITING_STATE_TEXT = 1 << 1
} PpsAnnotationEditingState;

PPS_PUBLIC
PpsDocumentModel *pps_document_model_new (void);
PPS_PUBLIC
PpsDocumentModel *pps_document_model_new_with_document (PpsDocument *document);

PPS_PUBLIC
void pps_document_model_set_document (PpsDocumentModel *model,
                                      PpsDocument *document);
PPS_PUBLIC
PpsDocument *pps_document_model_get_document (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_page (PpsDocumentModel *model,
                                  gint page);
PPS_PUBLIC
void pps_document_model_set_page_by_label (PpsDocumentModel *model,
                                           const gchar *page_label);
PPS_PUBLIC
gint pps_document_model_get_page (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_scale (PpsDocumentModel *model,
                                   gdouble scale);
PPS_PUBLIC
gdouble pps_document_model_get_scale (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_max_scale (PpsDocumentModel *model,
                                       gdouble max_scale);
PPS_PUBLIC
gdouble pps_document_model_get_max_scale (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_min_scale (PpsDocumentModel *model,
                                       gdouble min_scale);
PPS_PUBLIC
gdouble pps_document_model_get_min_scale (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_sizing_mode (PpsDocumentModel *model,
                                         PpsSizingMode mode);
PPS_PUBLIC
PpsSizingMode pps_document_model_get_sizing_mode (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_page_layout (PpsDocumentModel *model,
                                         PpsPageLayout layout);
PPS_PUBLIC
PpsPageLayout pps_document_model_get_page_layout (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_rotation (PpsDocumentModel *model,
                                      gint rotation);
PPS_PUBLIC
gint pps_document_model_get_rotation (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_inverted_colors (PpsDocumentModel *model,
                                             gboolean inverted_colors);
PPS_PUBLIC
gboolean pps_document_model_get_inverted_colors (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_continuous (PpsDocumentModel *model,
                                        gboolean continuous);
PPS_PUBLIC
gboolean pps_document_model_get_continuous (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_dual_page_odd_pages_left (PpsDocumentModel *model,
                                                      gboolean odd_left);
PPS_PUBLIC
gboolean pps_document_model_get_dual_page_odd_pages_left (PpsDocumentModel *model);
PPS_PUBLIC
void pps_document_model_set_rtl (PpsDocumentModel *model,
                                 gboolean rtl);
PPS_PUBLIC
gboolean pps_document_model_get_rtl (PpsDocumentModel *model);

PPS_PUBLIC
void
pps_document_model_set_annotation_editing_state (PpsDocumentModel *model,
                                                 PpsAnnotationEditingState state);
PPS_PUBLIC
PpsAnnotationEditingState
pps_document_model_get_annotation_editing_state (PpsDocumentModel *model);

G_END_DECLS
