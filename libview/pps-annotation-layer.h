// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-layers.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2008 Carlos Garcia Campos  <carlosgc@gnome.org>
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include <gtk/gtk.h>

#include "pps-annotation.h"
#include "pps-annotations-context.h"
#include "pps-document-model.h"

#include <stdbool.h>

#define PPS_TYPE_ANNOTATION_LAYER (pps_annotation_layer_get_type ())

G_DECLARE_DERIVABLE_TYPE (PpsAnnotationLayer, pps_annotation_layer, PPS, ANNOTATION_LAYER, GtkWidget)
struct _PpsAnnotationLayerClass {
	GtkBoxClass parent_class;

	bool (*annot_added) (PpsAnnotationLayer *layer, PpsAnnotation *annot);
	void (*annot_removed) (PpsAnnotationLayer *layer, PpsAnnotation *annot);
	void (*clear_page) (PpsAnnotationLayer *layer);
};

gint
pps_annotation_layer_get_page (PpsAnnotationLayer *draw);

void
pps_annotation_layer_set_page (PpsAnnotationLayer *draw, int page);

PpsDocumentModel *
pps_annotation_layer_get_doc_model (PpsAnnotationLayer *layer);

PpsAnnotationsContext *
pps_annotation_layer_get_annotations_context (PpsAnnotationLayer *layer);
