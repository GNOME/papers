// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2000-2003 Marco Pesenti Gritti
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <glib-object.h>
#include <glib.h>

#include "pps-document.h"
#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_SELECTION (pps_selection_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsSelection, pps_selection, PPS, SELECTION, GObject)

typedef enum {
	PPS_SELECTION_STYLE_GLYPH,
	PPS_SELECTION_STYLE_WORD,
	PPS_SELECTION_STYLE_LINE
} PpsSelectionStyle;

struct _PpsSelectionInterface {
	GTypeInterface base_iface;

	void (*render_selection) (PpsSelection *selection,
	                          PpsRenderContext *rc,
	                          cairo_surface_t **surface,
	                          PpsRectangle *points,
	                          PpsRectangle *old_points,
	                          PpsSelectionStyle style,
	                          GdkRGBA *text,
	                          GdkRGBA *base);
	gchar *(*get_selected_text) (PpsSelection *selection,
	                             PpsPage *page,
	                             PpsSelectionStyle style,
	                             PpsRectangle *points);
	cairo_region_t *(*get_selection_region) (PpsSelection *selection,
	                                         PpsRenderContext *rc,
	                                         PpsSelectionStyle style,
	                                         PpsRectangle *points);
};

PPS_PUBLIC
void pps_selection_render_selection (PpsSelection *selection,
                                     PpsRenderContext *rc,
                                     cairo_surface_t **surface,
                                     PpsRectangle *points,
                                     PpsRectangle *old_points,
                                     PpsSelectionStyle style,
                                     GdkRGBA *text,
                                     GdkRGBA *base);
PPS_PUBLIC
gchar *pps_selection_get_selected_text (PpsSelection *selection,
                                        PpsPage *page,
                                        PpsSelectionStyle style,
                                        PpsRectangle *points);
PPS_PUBLIC
cairo_region_t *pps_selection_get_selection_region (PpsSelection *selection,
                                                    PpsRenderContext *rc,
                                                    PpsSelectionStyle style,
                                                    PpsRectangle *points);

G_END_DECLS
