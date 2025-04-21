// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2005 Jonathan Blandford <jrb@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-macros.h"
#include "pps-page.h"

G_BEGIN_DECLS

#define PPS_TYPE_RENDER_CONTEXT (pps_render_context_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsRenderContext, pps_render_context, PPS, RENDER_CONTEXT, GObject)

/* These are the same flags as poppler ones in glib/poppler.h and must be
 * kept in sync */
typedef enum {
	PPS_RENDER_ANNOTS_NONE = 0,
	PPS_RENDER_ANNOTS_TEXT = 1 << 0,
	PPS_RENDER_ANNOTS_LINK = 1 << 1,
	PPS_RENDER_ANNOTS_FREETEXT = 1 << 2,
	PPS_RENDER_ANNOTS_LINE = 1 << 3,
	PPS_RENDER_ANNOTS_SQUARE = 1 << 4,
	PPS_RENDER_ANNOTS_CIRCLE = 1 << 5,
	PPS_RENDER_ANNOTS_POLYGON = 1 << 6,
	PPS_RENDER_ANNOTS_POLYLINE = 1 << 7,
	PPS_RENDER_ANNOTS_HIGHLIGHT = 1 << 8,
	PPS_RENDER_ANNOTS_UNDERLINE = 1 << 9,
	PPS_RENDER_ANNOTS_SQUIGGLY = 1 << 10,
	PPS_RENDER_ANNOTS_STRIKEOUT = 1 << 11,
	PPS_RENDER_ANNOTS_STAMP = 1 << 12,
	PPS_RENDER_ANNOTS_CARET = 1 << 13,
	PPS_RENDER_ANNOTS_INK = 1 << 14,
	PPS_RENDER_ANNOTS_POPUP = 1 << 15,
	PPS_RENDER_ANNOTS_FILEATTACHMENT = 1 << 16,
	PPS_RENDER_ANNOTS_SOUND = 1 << 17,
	PPS_RENDER_ANNOTS_MOVIE = 1 << 18,
	PPS_RENDER_ANNOTS_WIDGET = 1 << 19,
	PPS_RENDER_ANNOTS_SCREEN = 1 << 20,
	PPS_RENDER_ANNOTS_PRINTERMARK = 1 << 21,
	PPS_RENDER_ANNOTS_TRAPNET = 1 << 22,
	PPS_RENDER_ANNOTS_WATERMARK = 1 << 23,
	PPS_RENDER_ANNOTS_3D = 1 << 24,
	PPS_RENDER_ANNOTS_RICHMEDIA = 1 << 25,

	/* Everything below are special flags to combine them all */
	PPS_RENDER_ANNOTS_PRINT_DOCUMENT = PPS_RENDER_ANNOTS_WIDGET,
	PPS_RENDER_ANNOTS_PRINT_MARKUP = ~(PPS_RENDER_ANNOTS_LINK | PPS_RENDER_ANNOTS_POPUP | PPS_RENDER_ANNOTS_MOVIE | PPS_RENDER_ANNOTS_SCREEN | PPS_RENDER_ANNOTS_PRINTERMARK | PPS_RENDER_ANNOTS_TRAPNET | PPS_RENDER_ANNOTS_WATERMARK | PPS_RENDER_ANNOTS_3D),
	PPS_RENDER_ANNOTS_PRINT_STAMP = PPS_RENDER_ANNOTS_WIDGET | PPS_RENDER_ANNOTS_STAMP,
	PPS_RENDER_ANNOTS_PRINT_ALL = PPS_RENDER_ANNOTS_PRINT_MARKUP,
	/* Enable all flags, by shifting and substracting the last one */
	PPS_RENDER_ANNOTS_ALL = (PPS_RENDER_ANNOTS_RICHMEDIA << 1) - 1
} PpsRenderAnnotsFlags;

struct _PpsRenderContext {
	GObject parent;

	PpsPage *page;
	gint rotation;
	gdouble scale;
	gint target_width;
	gint target_height;
	PpsRenderAnnotsFlags annot_flags;
};

PPS_PUBLIC
PpsRenderContext *pps_render_context_new (PpsPage *page,
                                          gint rotation,
                                          gdouble scale,
                                          PpsRenderAnnotsFlags annot_flags);
PPS_PUBLIC
void pps_render_context_set_page (PpsRenderContext *rc,
                                  PpsPage *page);
PPS_PUBLIC
void pps_render_context_set_rotation (PpsRenderContext *rc,
                                      gint rotation);
PPS_PUBLIC
void pps_render_context_set_scale (PpsRenderContext *rc,
                                   gdouble scale);
PPS_PUBLIC
void pps_render_context_set_target_size (PpsRenderContext *rc,
                                         int target_width,
                                         int target_height);
PPS_PUBLIC
void pps_render_context_compute_scaled_size (PpsRenderContext *rc,
                                             double width_points,
                                             double height_points,
                                             int *scaled_width,
                                             int *scaled_height);
PPS_PUBLIC
void pps_render_context_compute_transformed_size (PpsRenderContext *rc,
                                                  double width_points,
                                                  double height_points,
                                                  int *transformed_width,
                                                  int *transformed_height);
PPS_PUBLIC
void pps_render_context_compute_scales (PpsRenderContext *rc,
                                        double width_points,
                                        double height_points,
                                        double *scale_x,
                                        double *scale_y);

G_END_DECLS
