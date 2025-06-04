/* pps-annotations-context.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Pablo Correa Gomez <ablocorrea@hotmail.com>
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

#include "pps-undo-context.h"
#include <glib-object.h>

#include "pps-document-model.h"

G_BEGIN_DECLS

#define PPS_TYPE_ANNOTATIONS_CONTEXT (pps_annotations_context_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsAnnotationsContext, pps_annotations_context, PPS, ANNOTATIONS_CONTEXT, GObject)

struct _PpsAnnotationsContext {
	GObject parent_instance;
};

struct _PpsAnnotationsContextClass {
	GObjectClass parent_class;
};

PPS_PUBLIC PpsAnnotationsContext *
pps_annotations_context_new (PpsDocumentModel *model, PpsUndoContext *undo_context);
PPS_PUBLIC
GListModel *pps_annotations_context_get_annots_model (PpsAnnotationsContext *self);
PPS_PUBLIC
PpsAnnotation *pps_annotations_context_add_annotation_sync (PpsAnnotationsContext *self,
                                                            gint page_index,
                                                            PpsAnnotationType type,
                                                            const PpsPoint *start,
                                                            const PpsPoint *end,
                                                            const GdkRGBA *color,
                                                            const gpointer user_data);
PPS_PUBLIC
void pps_annotations_context_remove_annotation (PpsAnnotationsContext *self,
                                                PpsAnnotation *annot);
PPS_PUBLIC
void pps_annotations_context_set_color (PpsAnnotationsContext *self,
                                        const GdkRGBA *color);

PpsAnnotation *pps_annotations_context_get_annot_at_doc_point (PpsAnnotationsContext *self,
                                                               const PpsDocumentPoint *doc_point);

G_END_DECLS
