// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-layer-shape.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2026
 *
 * Implements drag-to-draw geometric shape annotations (line, rectangle,
 * circle/ellipse, arrow) stored as ink annotation paths.
 */

#include "pps-annotation-layer-shape.h"

#include <cairo.h>
#include <math.h>

#include "pps-annotation-model.h"
#include "pps-annotations-context.h"

/* Number of points used to approximate an ellipse */
#define ELLIPSE_N_POINTS 64

/* Minimum drag distance (px) to create an annotation; below this a default
 * size is used so an accidental click still produces something usable.    */
#define MIN_DRAG_PX 8.0

/* Arrowhead size as a multiple of the stroke line_width, clamped below */
#define ARROW_HEAD_SCALE 6.0
#define ARROW_HEAD_MIN   12.0

typedef struct {
	gdouble drag_start_x;
	gdouble drag_start_y;
	gdouble drag_offset_x; /* accumulated offset from GtkGestureDrag */
	gdouble drag_offset_y;
	gboolean dragging;
} PpsAnnotationLayerShapePrivate;

struct _PpsAnnotationLayerShape {
	PpsAnnotationLayer base_instance;
};

G_DEFINE_TYPE_WITH_PRIVATE (PpsAnnotationLayerShape, pps_annotation_layer_shape, PPS_TYPE_ANNOTATION_LAYER)
#define SHAPE_GET_PRIVATE(o) pps_annotation_layer_shape_get_instance_private (o)

#define GET_ANNOT_MODEL(d) pps_document_model_get_annotation_model (pps_annotation_layer_get_doc_model (PPS_ANNOTATION_LAYER (d)))
#define GET_DOC_MODEL(d)   pps_annotation_layer_get_doc_model (PPS_ANNOTATION_LAYER (d))
#define GET_ANNOT_CONTEXT(d) pps_annotation_layer_get_annotations_context (PPS_ANNOTATION_LAYER (d))

/* ── Point / ink-list helpers ───────────────────────────────────────────── */

static PpsPath *
make_path (const PpsPoint *pts, gsize n)
{
	return pps_path_new_for_array (pts, (guint) n);
}

static PpsInkList *
make_line_ink (gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	PpsPoint pts[2] = { { x1, y1 }, { x2, y2 } };
	PpsPath *path = make_path (pts, 2);
	GSList *list = g_slist_prepend (NULL, path);
	PpsInkList *ink = pps_ink_list_new_for_list (list);
	g_slist_free (list);
	return ink;
}

static PpsInkList *
make_rect_ink (gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	PpsPoint pts[5] = {
		{ x1, y1 }, { x2, y1 }, { x2, y2 }, { x1, y2 }, { x1, y1 }
	};
	PpsPath *path = make_path (pts, 5);
	GSList *list = g_slist_prepend (NULL, path);
	PpsInkList *ink = pps_ink_list_new_for_list (list);
	g_slist_free (list);
	return ink;
}

static PpsInkList *
make_ellipse_ink (gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	gdouble cx = (x1 + x2) / 2.0;
	gdouble cy = (y1 + y2) / 2.0;
	gdouble rx = fabs (x2 - x1) / 2.0;
	gdouble ry = fabs (y2 - y1) / 2.0;
	PpsPoint pts[ELLIPSE_N_POINTS + 1];

	for (int i = 0; i <= ELLIPSE_N_POINTS; i++) {
		gdouble theta = (gdouble) i / ELLIPSE_N_POINTS * 2.0 * G_PI;
		pts[i].x = cx + rx * cos (theta);
		pts[i].y = cy + ry * sin (theta);
	}
	/* Explicitly close: sin(2π) ≈ -2.4e-16 in double precision, so pts[N]
	 * would not exactly equal pts[0] unless we pin it here. */
	pts[ELLIPSE_N_POINTS] = pts[0];

	PpsPath *path = make_path (pts, ELLIPSE_N_POINTS + 1);
	GSList *list = g_slist_prepend (NULL, path);
	PpsInkList *ink = pps_ink_list_new_for_list (list);
	g_slist_free (list);
	return ink;
}

static PpsInkList *
make_arrow_ink (gdouble x1, gdouble y1, gdouble x2, gdouble y2,
                gdouble head_size)
{
	gdouble dx = x2 - x1;
	gdouble dy = y2 - y1;
	gdouble len = sqrt (dx * dx + dy * dy);

	if (len < 0.1) {
		/* Degenerate arrow — just store as a line */
		PpsPoint pts[2] = { { x1, y1 }, { x2, y2 } };
		PpsPath   *path = make_path (pts, 2);
		GSList    *list = g_slist_prepend (NULL, path);
		PpsInkList *ink = pps_ink_list_new_for_list (list);
		g_slist_free (list);
		return ink;
	}

	gdouble ux = dx / len; /* unit vector along shaft */
	gdouble uy = dy / len;
	gdouble px = -uy;      /* perpendicular */
	gdouble py = ux;
	gdouble h  = head_size;
	gdouble w  = h * 0.4;

	/* Single polyline: shaft-start → tip → wing1 → wing2 → tip.
	 * Shaft and arrowhead share a MITER JOIN at the tip rather than
	 * two separate butt-caps, so the tip is always sharp and connected. */
	PpsPoint pts[5] = {
		{ x1, y1 },                                    /* shaft start */
		{ x2, y2 },                                    /* tip         */
		{ x2 - ux * h + px * w, y2 - uy * h + py * w }, /* wing 1   */
		{ x2 - ux * h - px * w, y2 - uy * h - py * w }, /* wing 2   */
		{ x2, y2 },                                    /* back to tip */
	};
	PpsPath   *path = make_path (pts, 5);
	GSList    *list = g_slist_prepend (NULL, path);
	PpsInkList *ink = pps_ink_list_new_for_list (list);
	g_slist_free (list);
	return ink;
}

/* ── Snapshot (live preview while dragging) ─────────────────────────────── */

static void
draw_arrow_preview (cairo_t *cr,
                    gdouble x1, gdouble y1,
                    gdouble x2, gdouble y2,
                    gdouble head_size)
{
	gdouble dx  = x2 - x1;
	gdouble dy  = y2 - y1;
	gdouble len = sqrt (dx * dx + dy * dy);

	/* Draw shaft + open V-head as one continuous polyline, matching the
	 * stored annotation format: [start, tip, wing1, wing2, tip]. */
	cairo_move_to (cr, x1, y1);
	cairo_line_to (cr, x2, y2);

	if (len > 0.1) {
		gdouble ux = dx / len;
		gdouble uy = dy / len;
		gdouble px = -uy;
		gdouble py = ux;
		gdouble h  = head_size;
		gdouble w  = h * 0.4;

		cairo_line_to (cr, x2 - ux * h + px * w, y2 - uy * h + py * w);
		cairo_line_to (cr, x2 - ux * h - px * w, y2 - uy * h - py * w);
		cairo_line_to (cr, x2, y2);
	}
	cairo_stroke (cr);
}

static void
pps_annotation_layer_shape_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
	PpsAnnotationLayerShape *self = PPS_ANNOTATION_LAYER_SHAPE (widget);
	PpsAnnotationLayerShapePrivate *priv = SHAPE_GET_PRIVATE (self);

	if (!priv->dragging)
		return;

	int width  = gtk_widget_get_width  (widget);
	int height = gtk_widget_get_height (widget);

	cairo_t *cr = gtk_snapshot_append_cairo (
	    snapshot, &GRAPHENE_RECT_INIT (0, 0, width, height));

	GdkRGBA *color = pps_annotation_model_get_pen_color (GET_ANNOT_MODEL (self));
	gdouble lw     = pps_annotation_model_get_pen_radius (GET_ANNOT_MODEL (self));

	gdk_cairo_set_source_rgba (cr, color);
	cairo_set_line_width (cr, MAX (lw, 1.0));
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

	gdouble x1 = priv->drag_start_x;
	gdouble y1 = priv->drag_start_y;
	gdouble x2 = priv->drag_start_x + priv->drag_offset_x;
	gdouble y2 = priv->drag_start_y + priv->drag_offset_y;

	PpsAnnotationTool tool = pps_annotation_model_get_tool (GET_ANNOT_MODEL (self));

	switch (tool) {
	case TOOL_LINE:
		cairo_move_to (cr, x1, y1);
		cairo_line_to (cr, x2, y2);
		cairo_stroke (cr);
		break;

	case TOOL_RECTANGLE: {
		gdouble rx = MIN (x1, x2);
		gdouble ry = MIN (y1, y2);
		gdouble rw = fabs (x2 - x1);
		gdouble rh = fabs (y2 - y1);
		cairo_rectangle (cr, rx, ry, rw, rh);
		cairo_stroke (cr);
		break;
	}

	case TOOL_CIRCLE: {
		gdouble cx = (x1 + x2) / 2.0;
		gdouble cy = (y1 + y2) / 2.0;
		gdouble rx = fabs (x2 - x1) / 2.0;
		gdouble ry = fabs (y2 - y1) / 2.0;
		if (rx < 1.0 || ry < 1.0)
			break;
		cairo_save (cr);
		cairo_translate (cr, cx, cy);
		cairo_scale (cr, rx, ry);
		cairo_arc (cr, 0.0, 0.0, 1.0, 0.0, 2.0 * G_PI);
		cairo_restore (cr);
		cairo_stroke (cr);
		break;
	}

	case TOOL_ARROW: {
		gdouble head_size = MAX (ARROW_HEAD_MIN, lw * ARROW_HEAD_SCALE);
		draw_arrow_preview (cr, x1, y1, x2, y2, head_size);
		break;
	}

	default:
		break;
	}

	cairo_destroy (cr);
}

/* ── Drag gesture callbacks ─────────────────────────────────────────────── */

static void
on_drag_begin (GtkGestureDrag *gesture,
               gdouble         x,
               gdouble         y,
               PpsAnnotationLayerShape *self)
{
	PpsAnnotationLayerShapePrivate *priv = SHAPE_GET_PRIVATE (self);
	gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);

	priv->drag_start_x  = x;
	priv->drag_start_y  = y;
	priv->drag_offset_x = 0.0;
	priv->drag_offset_y = 0.0;
	priv->dragging      = TRUE;

	gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_drag_update (GtkGestureDrag *gesture,
                gdouble         offset_x,
                gdouble         offset_y,
                PpsAnnotationLayerShape *self)
{
	PpsAnnotationLayerShapePrivate *priv = SHAPE_GET_PRIVATE (self);
	priv->drag_offset_x = offset_x;
	priv->drag_offset_y = offset_y;
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_drag_end (GtkGestureDrag *gesture,
             gdouble         offset_x,
             gdouble         offset_y,
             PpsAnnotationLayerShape *self)
{
	PpsAnnotationLayerShapePrivate *priv = SHAPE_GET_PRIVATE (self);
	PpsAnnotationsContext *context = GET_ANNOT_CONTEXT (self);
	PpsAnnotationModel *annot_model = GET_ANNOT_MODEL (self);
	gdouble scale = pps_document_model_get_scale (GET_DOC_MODEL (self));

	priv->dragging = FALSE;
	gtk_widget_queue_draw (GTK_WIDGET (self));

	/* Widget-space coordinates */
	gdouble wx1 = priv->drag_start_x;
	gdouble wy1 = priv->drag_start_y;
	gdouble wx2 = wx1 + offset_x;
	gdouble wy2 = wy1 + offset_y;

	/* Apply minimum size for accidental single-clicks */
	if (fabs (wx2 - wx1) < MIN_DRAG_PX && fabs (wy2 - wy1) < MIN_DRAG_PX) {
		wx2 = wx1 + MIN_DRAG_PX;
		wy2 = wy1 + MIN_DRAG_PX;
	}

	/* Get page height for coordinate flip.
	 * PDF Y=0 is at the bottom of the page; widget Y=0 is at the top.
	 * doc_y = page_height - widget_y / scale  (same as pps-annotation-layer-ink.c) */
	gdouble page_height;
	pps_document_get_page_size (
	    pps_document_model_get_document (GET_DOC_MODEL (self)),
	    pps_annotation_layer_get_page (PPS_ANNOTATION_LAYER (self)),
	    NULL, &page_height);

	GdkRGBA *color   = pps_annotation_model_get_pen_color (annot_model);
	gdouble lw        = pps_annotation_model_get_pen_radius (annot_model);
	PpsAnnotationTool tool = pps_annotation_model_get_tool (annot_model);

	PpsInkList *ink = NULL;

	switch (tool) {
	case TOOL_LINE:
	case TOOL_ARROW: {
		/* Direction is intentional — flip each endpoint individually */
		gdouble dx1 = wx1 / scale;
		gdouble dy1 = page_height - wy1 / scale;
		gdouble dx2 = wx2 / scale;
		gdouble dy2 = page_height - wy2 / scale;
		if (tool == TOOL_LINE) {
			ink = make_line_ink (dx1, dy1, dx2, dy2);
		} else {
			gdouble head_size_doc = MAX (ARROW_HEAD_MIN / scale,
			                             (lw * ARROW_HEAD_SCALE) / scale);
			ink = make_arrow_ink (dx1, dy1, dx2, dy2, head_size_doc);
		}
		break;
	}
	case TOOL_RECTANGLE:
	case TOOL_CIRCLE: {
		/* Normalise in widget space FIRST, then flip.
		 * wyMin (top of drag in screen) maps to dyMax (top in PDF = larger Y).
		 * wyMax (bottom of drag) maps to dyMin.  */
		gdouble wxMin = MIN (wx1, wx2);
		gdouble wxMax = MAX (wx1, wx2);
		gdouble wyMin = MIN (wy1, wy2);
		gdouble wyMax = MAX (wy1, wy2);
		gdouble dxMin = wxMin / scale;
		gdouble dyMin = page_height - wyMax / scale;
		gdouble dxMax = wxMax / scale;
		gdouble dyMax = page_height - wyMin / scale;
		if (tool == TOOL_RECTANGLE)
			ink = make_rect_ink (dxMin, dyMin, dxMax, dyMax);
		else
			ink = make_ellipse_ink (dxMin, dyMin, dxMax, dyMax);
		break;
	}
	default:
		return;
	}

	if (!ink)
		return;

	PpsAnnotationInkAddData add_data = {
		.ink_list   = ink,
		.times      = NULL,
		.highlight  = FALSE,
		.line_width = (float) (2.0 * lw),   /* match ink layer: 2 * radius */
	};

	pps_annotations_context_add_annotation_sync (
	    context,
	    pps_annotation_layer_get_page (PPS_ANNOTATION_LAYER (self)),
	    PPS_ANNOTATION_TYPE_INK,
	    NULL, NULL,
	    color,
	    &add_data);

	pps_ink_list_free (ink);
}

/* ── GObject boilerplate ────────────────────────────────────────────────── */

static void
pps_annotation_layer_shape_init (PpsAnnotationLayerShape *self)
{
	PpsAnnotationLayerShapePrivate *priv = SHAPE_GET_PRIVATE (self);

	priv->dragging = FALSE;

	gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "crosshair");

	GtkGestureDrag *drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (drag), 1);

	g_signal_connect (drag, "drag-begin",  G_CALLBACK (on_drag_begin),  self);
	g_signal_connect (drag, "drag-update", G_CALLBACK (on_drag_update), self);
	g_signal_connect (drag, "drag-end",    G_CALLBACK (on_drag_end),    self);

	gtk_widget_add_controller (GTK_WIDGET (self),
	                           GTK_EVENT_CONTROLLER (drag));
}

static void
pps_annotation_layer_shape_class_init (PpsAnnotationLayerShapeClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->snapshot = pps_annotation_layer_shape_snapshot;
}

GtkWidget *
pps_annotation_layer_shape_new (PpsDocument           *document,
                                 PpsDocumentModel      *model,
                                 PpsAnnotationsContext *annotations_context)
{
	g_return_val_if_fail (PPS_IS_DOCUMENT (document), NULL);

	return g_object_new (PPS_TYPE_ANNOTATION_LAYER_SHAPE,
	                     "model",               model,
	                     "annotations-context", annotations_context,
	                     NULL);
}
