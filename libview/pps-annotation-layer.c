// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-layer.c
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#include "pps-annotation-layer.h"
#include "pps-view-private.h"
#include "pps-view.h"

typedef struct {
	PpsDocumentModel *model;
	PpsAnnotationsContext *annotations_context;

	gint page;
} PpsAnnotationLayerPrivate;

enum { PROP_0,
       PROP_DOCUMENT_MODEL,
       PROP_ANNOTATIONS_CONTEXT };

G_DEFINE_TYPE_WITH_PRIVATE (PpsAnnotationLayer, pps_annotation_layer, GTK_TYPE_WIDGET)
#define LAYER_GET_PRIVATE(o) pps_annotation_layer_get_instance_private (o)

PpsAnnotationsContext *
pps_annotation_layer_get_annotations_context (PpsAnnotationLayer *layer)
{
	PpsAnnotationLayerPrivate *priv = LAYER_GET_PRIVATE (layer);
	return priv->annotations_context;
}

static void
pps_annotation_layer_init (PpsAnnotationLayer *draw)
{
}

static void
annot_removed (PpsAnnotationLayer *layer, PpsAnnotation *annotation)
{
	PpsAnnotationLayerPrivate *priv = LAYER_GET_PRIVATE (layer);
	PpsAnnotationLayerClass *class = PPS_ANNOTATION_LAYER_GET_CLASS (layer);
	if (pps_annotation_get_page_index (annotation) == priv->page && class->annot_removed) {
		class->annot_removed (layer, annotation);
	}
}

static void
annot_added (PpsAnnotationLayer *layer, PpsAnnotation *annotation)
{
	PpsAnnotationLayerPrivate *priv = LAYER_GET_PRIVATE (layer);
	PpsAnnotationLayerClass *class = PPS_ANNOTATION_LAYER_GET_CLASS (layer);
	if (pps_annotation_get_page_index (annotation) == priv->page && class->annot_added) {
		class->annot_added (layer, annotation);
	}
}

static void
annots_loaded (PpsAnnotationLayer *layer, gpointer data)
{
	PpsAnnotationLayerPrivate *priv = LAYER_GET_PRIVATE (layer);
	int page = priv->page;
	priv->page = -1;
	pps_annotation_layer_set_page (layer, page);
}

static GObject *
pps_annotation_layer_constructor (GType type,
                                  guint n_construct_properties,
                                  GObjectConstructParam *construct_params)
{
	GObject *object;
	PpsAnnotationLayer *layer;
	PpsAnnotationLayerPrivate *priv;
	int page;

	object = G_OBJECT_CLASS (pps_annotation_layer_parent_class)
	             ->constructor (type, n_construct_properties, construct_params);

	layer = PPS_ANNOTATION_LAYER (object);
	priv = LAYER_GET_PRIVATE (layer);
	/* pretend it is a new page to load annotations */
	page = priv->page;
	priv->page = -1;
	pps_annotation_layer_set_page (layer, page);

	g_signal_connect_object (priv->annotations_context, "annot-removed", G_CALLBACK (annot_removed), layer, G_CONNECT_SWAPPED);
	g_signal_connect_object (priv->annotations_context, "annot-added", G_CALLBACK (annot_added), layer, G_CONNECT_SWAPPED);
	g_signal_connect_object (priv->annotations_context, "annots-loaded", G_CALLBACK (annots_loaded), layer, G_CONNECT_SWAPPED);

	return object;
}

void
pps_annotation_layer_set_page (PpsAnnotationLayer *layer, int page)
{
	PpsAnnotationLayerPrivate *priv = LAYER_GET_PRIVATE (layer);
	if (priv->page == page) {
		return;
	}

	priv->page = page;
	if (PPS_ANNOTATION_LAYER_GET_CLASS (layer)->clear_page) {
		PPS_ANNOTATION_LAYER_GET_CLASS (layer)->clear_page (layer);
	}

	GListModel *all_annots = pps_annotations_context_get_annots_model (priv->annotations_context);
	for (int i = 0; i < g_list_model_get_n_items (all_annots); i++) {
		PpsAnnotation *a = PPS_ANNOTATION (g_list_model_get_item (all_annots, i));
		annot_added (layer, a);
	}
}

PpsDocumentModel *
pps_annotation_layer_get_doc_model (PpsAnnotationLayer *layer)
{
	PpsAnnotationLayerPrivate *priv = LAYER_GET_PRIVATE (layer);
	return priv->model;
}

static void
pps_annotation_layer_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
	PpsAnnotationLayer *draw = PPS_ANNOTATION_LAYER (object);
	PpsAnnotationLayerPrivate *priv = LAYER_GET_PRIVATE (draw);

	switch (prop_id) {
	case PROP_DOCUMENT_MODEL:
		priv->model = g_value_get_object (value);
		break;
	case PROP_ANNOTATIONS_CONTEXT:
		priv->annotations_context = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

gint
pps_annotation_layer_get_page (PpsAnnotationLayer *draw)
{
	PpsAnnotationLayerPrivate *priv = LAYER_GET_PRIVATE (draw);

	return priv->page;
}

static void
pps_annotation_layer_class_init (PpsAnnotationLayerClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->constructor = pps_annotation_layer_constructor;
	g_object_class->set_property = pps_annotation_layer_set_property;

	g_object_class_install_property (g_object_class,
	                                 PROP_DOCUMENT_MODEL,
	                                 g_param_spec_object ("model",
	                                                      NULL,
	                                                      NULL,
	                                                      PPS_TYPE_DOCUMENT_MODEL,
	                                                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_ANNOTATIONS_CONTEXT,
	                                 g_param_spec_object ("annotations-context",
	                                                      NULL,
	                                                      NULL,
	                                                      PPS_TYPE_ANNOTATIONS_CONTEXT,
	                                                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}
