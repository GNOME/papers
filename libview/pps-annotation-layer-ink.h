// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-form-widget-factory.c
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include <gtk/gtk.h>

#include "pps-annotation-layer.h"
#include "pps-annotation.h"
#include "pps-document-model.h"

#define PPS_TYPE_ANNOTATION_LAYER_INK (pps_annotation_layer_ink_get_type ())
G_DECLARE_FINAL_TYPE (PpsAnnotationLayerInk, pps_annotation_layer_ink, PPS, ANNOTATION_LAYER_INK, PpsAnnotationLayer)

GtkWidget *pps_annotation_layer_ink_new (PpsDocument *document,
                                         PpsDocumentModel *model,
                                         PpsAnnotationsContext *annotations_context);

void pps_annotation_layer_ink_snapshot_below (PpsAnnotationLayerInk *layer, GtkSnapshot *snapshot);
G_END_DECLS
