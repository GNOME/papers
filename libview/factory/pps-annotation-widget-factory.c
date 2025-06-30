// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-widget-factory.c
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#include "pps-annotation-widget-factory.h"
#include "pps-annotation-overlay.h"

typedef struct
{
	PpsDocumentModel *model;
	PpsAnnotationsContext *annots_context;
	GHashTable *free_text_widgets;
} PpsAnnotationWidgetFactoryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsAnnotationWidgetFactory, pps_annotation_widget_factory, PPS_TYPE_ELEMENT_WIDGET_FACTORY)

#define GET_PRIVATE(o) pps_annotation_widget_factory_get_instance_private (o)

static void
pps_annotation_widget_factory_init (PpsAnnotationWidgetFactory *factory)
{
	PpsAnnotationWidgetFactoryPrivate *priv = GET_PRIVATE (factory);
	priv->free_text_widgets = g_hash_table_new (NULL, NULL);
}

static void
pps_annotation_widget_factory_dispose (GObject *object)
{
	PpsAnnotationWidgetFactory *factory = PPS_ANNOTATION_WIDGET_FACTORY (object);
	PpsAnnotationWidgetFactoryPrivate *priv = GET_PRIVATE (factory);

	g_clear_object (&priv->model);
	g_clear_object (&priv->annots_context);

	g_hash_table_destroy (priv->free_text_widgets);

	G_OBJECT_CLASS (pps_annotation_widget_factory_parent_class)->dispose (object);
}

static GtkWidget *
widget_for_annot (PpsAnnotationWidgetFactory *factory, PpsAnnotation *annot)
{
	PpsAnnotationWidgetFactoryPrivate *priv = GET_PRIVATE (factory);
	GtkWidget *annot_widget = NULL;

	if (PPS_IS_ANNOTATION_FREE_TEXT (annot)) {
		annot_widget = GTK_WIDGET (g_hash_table_lookup (priv->free_text_widgets, annot));
		if (!annot_widget) {
			annot_widget = pps_overlay_annotation_entry_new (annot,
			                                                 priv->annots_context,
			                                                 priv->model);
			g_hash_table_insert (priv->free_text_widgets, annot, g_object_ref_sink (annot_widget));
		}
	}

	return annot_widget;
}

static GList *
pps_annotation_widget_factory_widgets_for_page (PpsElementWidgetFactory *factory, guint page_index)
{
	PpsAnnotationWidgetFactory *annot_factory = PPS_ANNOTATION_WIDGET_FACTORY (factory);
	PpsAnnotationWidgetFactoryPrivate *priv = GET_PRIVATE (annot_factory);
	GListModel *all_annots;
	GList *widgets = NULL;

	all_annots = pps_annotations_context_get_annots_model (priv->annots_context);

	for (int i = pps_annotations_context_first_index_for_page (priv->annots_context, page_index);
	     i < g_list_model_get_n_items (all_annots); i++) {
		PpsAnnotation *annot = PPS_ANNOTATION (g_list_model_get_item (all_annots, i));
		GtkWidget *w;

		if (pps_annotation_get_page_index (annot) != page_index) {
			break;
		}
		w = widget_for_annot (annot_factory, annot);

		if (w)
			widgets = g_list_append (widgets, w);
	}
	return widgets;
}

static void
pps_annotation_widget_factory_annots_loaded (gpointer context,
                                             PpsAnnotationWidgetFactory *factory)
{
	pps_element_widget_factory_query_reload (PPS_ELEMENT_WIDGET_FACTORY (factory));
}

static void
pps_annotation_widget_factory_annots_added (PpsAnnotationWidgetFactory *factory, PpsAnnotation *a)
{
	GtkWidget *widget = widget_for_annot (factory, a);
	if (widget) {
		pps_element_widget_factory_new_widget_for_page (PPS_ELEMENT_WIDGET_FACTORY (factory),
		                                                pps_annotation_get_page_index (a),
		                                                widget);
		pps_overlay_annotation_grab_focus (PPS_OVERLAY_ANNOTATION (widget), 0, 0);
	}
}

static void
pps_annotation_widget_factory_annots_removed (PpsAnnotationWidgetFactory *factory, PpsAnnotation *annot)
{
	PpsAnnotationWidgetFactoryPrivate *priv = GET_PRIVATE (factory);

	GtkWidget *annot_widget = GTK_WIDGET (g_hash_table_lookup (priv->free_text_widgets, annot));
	if (annot_widget) {
		pps_element_widget_factory_widget_removed (PPS_ELEMENT_WIDGET_FACTORY (factory),
		                                           pps_annotation_get_page_index (annot),
		                                           annot_widget);
		g_hash_table_remove (priv->free_text_widgets, annot);
	}
}

static void
pps_annotation_widget_factory_setup (PpsElementWidgetFactory *element_factory,
                                     PpsDocumentModel *model,
                                     PpsAnnotationsContext *annots_context,
                                     PpsPixbufCache *pixbuf_cache,
                                     GPtrArray *page_widgets,
                                     PpsPageCache *page_cache)
{
	PpsAnnotationWidgetFactory *factory = PPS_ANNOTATION_WIDGET_FACTORY (element_factory);
	PpsAnnotationWidgetFactoryPrivate *priv = GET_PRIVATE (factory);

	if (priv->model != NULL)
		g_signal_handlers_disconnect_by_data (priv->model, factory);
	if (priv->annots_context != NULL)
		g_signal_handlers_disconnect_by_data (priv->annots_context, factory);

	g_set_object (&priv->model, model);
	g_set_object (&priv->annots_context, annots_context);

	g_signal_connect_object (priv->annots_context, "annots-loaded",
	                         G_CALLBACK (pps_annotation_widget_factory_annots_loaded),
	                         factory, G_CONNECT_DEFAULT);
	g_signal_connect_object (priv->annots_context, "annot-added",
	                         G_CALLBACK (pps_annotation_widget_factory_annots_added),
	                         factory, G_CONNECT_SWAPPED);
	g_signal_connect_object (priv->annots_context, "annot-removed",
	                         G_CALLBACK (pps_annotation_widget_factory_annots_removed),
	                         factory, G_CONNECT_SWAPPED);
}

static gboolean
pps_annotation_widget_factory_is_managed (PpsElementWidgetFactory *factory, GtkWidget *widget)
{
	return PPS_IS_OVERLAY_ANNOTATION (widget);
}

static void
pps_annotation_widget_factory_class_init (PpsAnnotationWidgetFactoryClass *page_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (page_class);
	PpsElementWidgetFactoryClass *factory_class = PPS_ELEMENT_WIDGET_FACTORY_CLASS (page_class);

	object_class->dispose = pps_annotation_widget_factory_dispose;

	factory_class->widgets_for_page = pps_annotation_widget_factory_widgets_for_page;
	factory_class->setup = pps_annotation_widget_factory_setup;
	factory_class->is_managed = pps_annotation_widget_factory_is_managed;
}

PpsElementWidgetFactory *
pps_annotation_widget_factory_new (void)
{
	return g_object_new (PPS_TYPE_ANNOTATION_WIDGET_FACTORY, NULL);
}
