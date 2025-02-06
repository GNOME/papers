// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-layer-objects.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */
#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include "pps-annotation-layer.h"

#define PPS_TYPE_ANNOTATION_LAYER_OBJECTS (pps_annotation_layer_objects_get_type ())
G_DECLARE_FINAL_TYPE (PpsAnnotationLayerObjects, pps_annotation_layer_objects, PPS, ANNOTATION_LAYER_OBJECTS, PpsAnnotationLayer)

GtkWidget *pps_annotation_layer_objects_new (PpsDocument *document,
                                             PpsDocumentModel *model,
                                             PpsAnnotationsContext *annotations_context);
G_END_DECLS
