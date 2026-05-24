// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-layer-objects.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#include "pps-annotation-layer-objects.h"

#include "gtk/gtk.h"
#include "pps-annotation-model.h"

/* Minimum drag size (pixels) before we switch to drag-defined rect;
 * below this we fall back to a sensible default text-box size.       */
#define MIN_DRAG_PX 12.0
/* Default text-box width in document points when user just clicks */
#define DEFAULT_BOX_WIDTH_PT  150.0
#define DEFAULT_BOX_HEIGHT_PT  30.0

typedef struct {
	gdouble drag_start_x;
	gdouble drag_start_y;
	gdouble drag_offset_x;
	gdouble drag_offset_y;
	gboolean dragging;
} PpsAnnotationLayerObjectsPrivate;

struct _PpsAnnotationLayerObjects {
	PpsAnnotationLayer base_instance;
};

enum { PROP_0 };

G_DEFINE_TYPE_WITH_PRIVATE (PpsAnnotationLayerObjects, pps_annotation_layer_objects, PPS_TYPE_ANNOTATION_LAYER)
#define OBJECTS_GET_PRIVATE(o) pps_annotation_layer_objects_get_instance_private (o)

#define GET_ANNOT_MODEL(d) pps_document_model_get_annotation_model (pps_annotation_layer_get_doc_model (PPS_ANNOTATION_LAYER (d)))

#define GET_DOC_MODEL(d) pps_annotation_layer_get_doc_model (PPS_ANNOTATION_LAYER (d))
#define GET_ANNOT_CONTEXT(d) pps_annotation_layer_get_annotations_context (PPS_ANNOTATION_LAYER (d))

static void
on_drag_begin (GtkGestureDrag *gesture,
               gdouble         x,
               gdouble         y,
               PpsAnnotationLayerObjects *layer)
{
	PpsAnnotationLayerObjectsPrivate *priv = OBJECTS_GET_PRIVATE (layer);
	gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);

	priv->drag_start_x  = x;
	priv->drag_start_y  = y;
	priv->drag_offset_x = 0.0;
	priv->drag_offset_y = 0.0;
	priv->dragging      = TRUE;

	gtk_widget_queue_draw (GTK_WIDGET (layer));
}

static void
on_drag_update (GtkGestureDrag *gesture,
                gdouble         offset_x,
                gdouble         offset_y,
                PpsAnnotationLayerObjects *layer)
{
	PpsAnnotationLayerObjectsPrivate *priv = OBJECTS_GET_PRIVATE (layer);
	priv->drag_offset_x = offset_x;
	priv->drag_offset_y = offset_y;
	gtk_widget_queue_draw (GTK_WIDGET (layer));
}

static void
on_drag_end (GtkGestureDrag *gesture,
             gdouble         offset_x,
             gdouble         offset_y,
             PpsAnnotationLayerObjects *layer)
{
	PpsAnnotationLayerObjectsPrivate *priv = OBJECTS_GET_PRIVATE (layer);
	PpsAnnotationsContext *context = GET_ANNOT_CONTEXT (layer);
	PpsAnnotationModel *annot_model = GET_ANNOT_MODEL (layer);
	gdouble scale = pps_document_model_get_scale (GET_DOC_MODEL (layer));

	priv->dragging = FALSE;
	gtk_widget_queue_draw (GTK_WIDGET (layer));

	/* Widget-space end point */
	gdouble wx2 = priv->drag_start_x + offset_x;
	gdouble wy2 = priv->drag_start_y + offset_y;

	/* If the drag was tiny (essentially a click), use a default box size */
	if (fabs (offset_x) < MIN_DRAG_PX && fabs (offset_y) < MIN_DRAG_PX) {
		wx2 = priv->drag_start_x + DEFAULT_BOX_WIDTH_PT  * scale;
		wy2 = priv->drag_start_y + DEFAULT_BOX_HEIGHT_PT * scale;
	}

	/* Normalise so top-left is always start */
	gdouble wx1 = MIN (priv->drag_start_x, wx2);
	gdouble wy1 = MIN (priv->drag_start_y, wy2);
	wx2 = MAX (priv->drag_start_x, wx2);
	wy2 = MAX (priv->drag_start_y, wy2);

	/* Convert to document coordinates */
	PpsPoint start = { wx1 / scale, wy1 / scale };
	PpsPoint end   = { wx2 / scale, wy2 / scale };

	GdkRGBA *color = pps_annotation_model_get_text_color (annot_model);

	pps_annotations_context_add_annotation_sync (
	    context,
	    pps_annotation_layer_get_page (PPS_ANNOTATION_LAYER (layer)),
	    PPS_ANNOTATION_TYPE_FREE_TEXT,
	    &start, &end,
	    color,
	    pps_annotation_model_get_font_desc (annot_model));
}

/* Draw a dashed preview rectangle during drag */
static void
pps_annotation_layer_objects_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
	PpsAnnotationLayerObjects *self = PPS_ANNOTATION_LAYER_OBJECTS (widget);
	PpsAnnotationLayerObjectsPrivate *priv = OBJECTS_GET_PRIVATE (self);

	if (!priv->dragging)
		return;

	int width  = gtk_widget_get_width  (widget);
	int height = gtk_widget_get_height (widget);

	cairo_t *cr = gtk_snapshot_append_cairo (
	    snapshot, &GRAPHENE_RECT_INIT (0, 0, width, height));

	gdouble x1 = MIN (priv->drag_start_x, priv->drag_start_x + priv->drag_offset_x);
	gdouble y1 = MIN (priv->drag_start_y, priv->drag_start_y + priv->drag_offset_y);
	gdouble x2 = MAX (priv->drag_start_x, priv->drag_start_x + priv->drag_offset_x);
	gdouble y2 = MAX (priv->drag_start_y, priv->drag_start_y + priv->drag_offset_y);

	/* Fill */
	cairo_set_source_rgba (cr, 0.0, 0.4, 0.9, 0.08);
	cairo_rectangle (cr, x1, y1, x2 - x1, y2 - y1);
	cairo_fill (cr);

	/* Dashed border */
	cairo_set_source_rgba (cr, 0.0, 0.4, 0.9, 0.7);
	cairo_set_line_width (cr, 1.5);
	const double dashes[] = { 4.0, 4.0 };
	cairo_set_dash (cr, dashes, 2, 0.0);
	cairo_rectangle (cr, x1, y1, x2 - x1, y2 - y1);
	cairo_stroke (cr);

	cairo_destroy (cr);
}

static void
pps_annotation_layer_objects_init (PpsAnnotationLayerObjects *draw)
{
	gtk_widget_set_cursor_from_name (GTK_WIDGET (draw), "text");

	GtkGestureDrag *drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (drag), 1);

	g_signal_connect (drag, "drag-begin",  G_CALLBACK (on_drag_begin),  draw);
	g_signal_connect (drag, "drag-update", G_CALLBACK (on_drag_update), draw);
	g_signal_connect (drag, "drag-end",    G_CALLBACK (on_drag_end),    draw);

	gtk_widget_add_controller (GTK_WIDGET (draw), GTK_EVENT_CONTROLLER (drag));
}

static void
pps_annotation_layer_objects_class_init (PpsAnnotationLayerObjectsClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->snapshot = pps_annotation_layer_objects_snapshot;
}

GtkWidget *
pps_annotation_layer_objects_new (PpsDocument *document,
                                  PpsDocumentModel *model,
                                  PpsAnnotationsContext *annotations_context)
{
	GtkWidget *draw;

	g_return_val_if_fail (PPS_IS_DOCUMENT (document), NULL);

	draw = g_object_new (PPS_TYPE_ANNOTATION_LAYER_OBJECTS,
	                     "model",
	                     model,
	                     "annotations-context",
	                     annotations_context,
	                     NULL);

	return draw;
}
