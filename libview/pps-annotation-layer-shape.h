// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-layer-shape.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2026
 */
#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include "pps-annotation-layer.h"

#define PPS_TYPE_ANNOTATION_LAYER_SHAPE (pps_annotation_layer_shape_get_type ())
G_DECLARE_FINAL_TYPE (PpsAnnotationLayerShape, pps_annotation_layer_shape, PPS, ANNOTATION_LAYER_SHAPE, PpsAnnotationLayer)

GtkWidget *pps_annotation_layer_shape_new (PpsDocument *document,
                                           PpsDocumentModel *model,
                                           PpsAnnotationsContext *annotations_context);
