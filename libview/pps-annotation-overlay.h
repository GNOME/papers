// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-overlay.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include <gtk/gtk.h>

#include "context/pps-annotations-context.h"
#include "pps-annotation.h"
#include "pps-document-model.h"

#define ANNOT_WIDGET_BORDER_SIZE 2
#define ANNOT_WIDGET_PADDING 24

G_BEGIN_DECLS

#define PPS_TYPE_OVERLAY_ANNOTATION (pps_overlay_annotation_get_type ())
G_DECLARE_DERIVABLE_TYPE (PpsOverlayAnnotation, pps_overlay_annotation, PPS, OVERLAY_ANNOTATION, GtkBox)
void pps_overlay_annotation_grab_focus (PpsOverlayAnnotation *entry, int x, int y);

struct _PpsOverlayAnnotationClass {
	GtkBoxClass base_class;
	void (*grab_focus) (PpsOverlayAnnotation *overlay, int x, int y);
	gchar *(*style) (PpsOverlayAnnotation *overlay);

	gboolean resize_handle;
	gboolean drag_only_on_border;
};

#define PPS_TYPE_OVERLAY_ANNOTATION_IMAGE (pps_overlay_annotation_image_get_type ())
G_DECLARE_FINAL_TYPE (PpsOverlayAnnotationImage, pps_overlay_annotation_image, PPS, OVERLAY_ANNOTATION_IMAGE, PpsOverlayAnnotation)

PpsAnnotation *pps_overlay_annotation_get_annotation (PpsOverlayAnnotation *overlay);

GtkWidget *pps_overlay_annotation_image_new (PpsAnnotation *annot,
                                             PpsAnnotationsContext *annots_context,
                                             PpsDocumentModel *model);

#define PPS_TYPE_OVERLAY_ANNOTATION_ENTRY (pps_overlay_annotation_entry_get_type ())
G_DECLARE_FINAL_TYPE (PpsOverlayAnnotationEntry, pps_overlay_annotation_entry, PPS, OVERLAY_ANNOTATION_ENTRY, PpsOverlayAnnotation)

GtkWidget *pps_overlay_annotation_entry_new (PpsAnnotation *annot,
                                             PpsAnnotationsContext *annots_context,
                                             PpsDocumentModel *model);

G_END_DECLS
