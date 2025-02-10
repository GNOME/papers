// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-model.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <glib-object.h>
#include <papers-document.h>

G_BEGIN_DECLS

typedef enum {
	TOOL_PENCIL,
	TOOL_HIGHLIGHT,
	TOOL_ERASER,
	TOOL_TEXT,
	TOOL_MAX
} PpsAnnotationTool;

PPS_PUBLIC
#define PPS_TYPE_ANNOTATION_MODEL (pps_annotation_model_get_type ())

G_DECLARE_FINAL_TYPE (PpsAnnotationModel, pps_annotation_model, PPS, ANNOTATION_MODEL, GObject)

PPS_PUBLIC
PpsAnnotationModel *pps_annotation_model_new (void);

PPS_PUBLIC
PpsAnnotationTool pps_annotation_model_get_tool (PpsAnnotationModel *model);

PPS_PUBLIC
void pps_annotation_model_set_tool (PpsAnnotationModel *model, PpsAnnotationTool tool);

PPS_PUBLIC
gdouble pps_annotation_model_get_eraser_radius (PpsAnnotationModel *model);

PPS_PUBLIC
void pps_annotation_model_set_eraser_radius (PpsAnnotationModel *model, gdouble radius);

PPS_PUBLIC
gdouble pps_annotation_model_get_pen_radius (PpsAnnotationModel *model);

PPS_PUBLIC
void pps_annotation_model_set_pen_radius (PpsAnnotationModel *model, gdouble radius);

PPS_PUBLIC
gdouble pps_annotation_model_get_highlight_radius (PpsAnnotationModel *model);

PPS_PUBLIC
void pps_annotation_model_set_highlight_radius (PpsAnnotationModel *model, gdouble radius);

PPS_PUBLIC
void
pps_annotation_model_set_pen_color (PpsAnnotationModel *model,
                                    const GdkRGBA *pen_color);

PPS_PUBLIC
GdkRGBA *
pps_annotation_model_get_pen_color (PpsAnnotationModel *model);

PPS_PUBLIC
void
pps_annotation_model_set_text_color (PpsAnnotationModel *model,
                                     const GdkRGBA *color);

PPS_PUBLIC
GdkRGBA *
pps_annotation_model_get_text_color (PpsAnnotationModel *model);

PPS_PUBLIC
void
pps_annotation_model_set_highlight_color (PpsAnnotationModel *model,
                                          const GdkRGBA *pen_color);

PPS_PUBLIC
GdkRGBA *
pps_annotation_model_get_highlight_color (PpsAnnotationModel *model);

PPS_PUBLIC
void
pps_annotation_model_set_font_desc (PpsAnnotationModel *model,
                                    const PangoFontDescription *font_desc);
PPS_PUBLIC
void
pps_annotation_model_set_font_size (PpsAnnotationModel *model,
                                    gdouble font_size);

PPS_PUBLIC
PangoFontDescription *
pps_annotation_model_get_font_desc (PpsAnnotationModel *model);

PPS_PUBLIC
gboolean
pps_annotation_model_get_eraser_objects (PpsAnnotationModel *model);

G_END_DECLS
