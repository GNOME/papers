// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-element-widget-factory.c
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#include "pps-element-widget-factory.h"
#include "pps-overlay.h"
#include "pps-view-page.h"

typedef struct
{
	PpsDocumentModel *model;
	PpsPixbufCache *pixbuf_cache;

	/* This array of PpsViewPage is owned by the PpsView */
	GPtrArray *page_widgets;
} PpsElementWidgetFactoryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsElementWidgetFactory, pps_element_widget_factory, G_TYPE_OBJECT)

#define GET_PRIVATE(o) pps_element_widget_factory_get_instance_private (o)

static void
pps_element_widget_factory_init (PpsElementWidgetFactory *factory)
{
}

static void
pps_element_widget_factory_dispose (GObject *object)
{
	PpsElementWidgetFactory *factory = PPS_ELEMENT_WIDGET_FACTORY (object);
	PpsElementWidgetFactoryPrivate *priv = GET_PRIVATE (factory);
	PpsElementWidgetFactoryClass *factory_class = PPS_ELEMENT_WIDGET_FACTORY_GET_CLASS (factory);

	if (priv->model != NULL)
		g_signal_handlers_disconnect_by_data (priv->model, factory);

	if (priv->pixbuf_cache != NULL)
		g_signal_handlers_disconnect_by_data (priv->pixbuf_cache, factory);

	if (priv->page_widgets) {
		for (gint i = 0; i < priv->page_widgets->len; i++) {
			PpsViewPage *page = g_ptr_array_index (priv->page_widgets, i);
			GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (page));

			while (child) {
				GtkWidget *next = gtk_widget_get_next_sibling (child);
				if (factory_class->is_managed (factory, child)) {
					gtk_widget_unparent (child);
				}
				child = next;
			}
		}
	}

	g_clear_object (&priv->model);
	g_clear_object (&priv->pixbuf_cache);

	G_OBJECT_CLASS (pps_element_widget_factory_parent_class)->dispose (object);
}

static void
job_finished_cb (PpsPixbufCache *pixbuf_cache,
                 int finished_page,
                 PpsElementWidgetFactory *factory)
{
	PpsElementWidgetFactoryPrivate *priv = GET_PRIVATE (factory);

	for (int i = 0; i < priv->page_widgets->len; i++) {
		PpsViewPage *page = g_ptr_array_index (priv->page_widgets, i);

		if (pps_view_page_get_page (page) == finished_page) {
			GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (page));
			while (child) {
				if (PPS_IS_OVERLAY (child)) {
					pps_overlay_update_visibility_from_state (
					    PPS_OVERLAY (child),
					    pps_pixbuf_cache_rendered_state (priv->pixbuf_cache, finished_page));
				}
				child = gtk_widget_get_next_sibling (child);
			}
			break;
		}
	}
}

static void
acquire_widgets (PpsElementWidgetFactory *factory, PpsViewPage *view_page)
{
	PpsElementWidgetFactoryPrivate *priv = GET_PRIVATE (factory);
	GtkWidget *view_page_widget = GTK_WIDGET (view_page);
	int page_index = pps_view_page_get_page (view_page);
	GList *all_widgets, *widget_list;

	if (page_index < 0) {
		return;
	}

	all_widgets = widget_list =
	    PPS_ELEMENT_WIDGET_FACTORY_GET_CLASS (factory)->widgets_for_page (factory, page_index);

	while (all_widgets) {
		GtkWidget *child = GTK_WIDGET (all_widgets->data);
		/* in case there is a race and the widget is still in another PpsViewPage */
		if (gtk_widget_get_parent (child)) {
			g_object_ref (child);
			gtk_widget_unparent (child);
			gtk_widget_set_parent (child, view_page_widget);
			g_object_unref (child);
		} else {
			gtk_widget_set_parent (child, view_page_widget);
		}
		pps_overlay_update_visibility_from_state (
		    PPS_OVERLAY (child),
		    pps_pixbuf_cache_rendered_state (priv->pixbuf_cache, page_index));
		all_widgets = all_widgets->next;
	}

	g_list_free (widget_list);

	gtk_widget_queue_resize (view_page_widget);
	gtk_widget_queue_draw (view_page_widget);
}

static void
on_page_changed (PpsViewPage *view_page,
                 GParamSpec *spec,
                 PpsElementWidgetFactory *factory)
{
	GtkWidget *child;
	GtkWidget *view_page_widget = GTK_WIDGET (view_page);
	PpsElementWidgetFactoryClass *factory_class = PPS_ELEMENT_WIDGET_FACTORY_GET_CLASS (factory);

	child = gtk_widget_get_first_child (view_page_widget);

	while (child) {
		GtkWidget *next = gtk_widget_get_next_sibling (child);
		if (factory_class->is_managed (factory, child)) {
			gtk_widget_unparent (child);
		}
		child = next;
	}

	if (pps_view_page_get_page (view_page) >= 0) {
		acquire_widgets (factory, view_page);
	}
}

void
pps_element_widget_factory_query_reload (PpsElementWidgetFactory *factory)
{
	PpsElementWidgetFactoryPrivate *priv = GET_PRIVATE (factory);

	for (int i = 0; i < priv->page_widgets->len; i++) {
		PpsViewPage *page = g_ptr_array_index (priv->page_widgets, i);
		acquire_widgets (factory, page);
	}
}

void
pps_element_widget_factory_new_widget_for_page (PpsElementWidgetFactory *factory,
                                                guint page_index,
                                                GtkWidget *widget)
{
	PpsElementWidgetFactoryPrivate *priv = GET_PRIVATE (factory);

	for (int i = 0; i < priv->page_widgets->len; i++) {
		PpsViewPage *page = g_ptr_array_index (priv->page_widgets, i);

		if (pps_view_page_get_page (page) == page_index) {
			gtk_widget_set_parent (widget, GTK_WIDGET (page));
			pps_overlay_update_visibility_from_state (
			    PPS_OVERLAY (widget),
			    pps_pixbuf_cache_rendered_state (priv->pixbuf_cache, page_index));
			break;
		}
	}
}

void
pps_element_widget_factory_widget_removed (PpsElementWidgetFactory *factory,
                                           guint page_index,
                                           GtkWidget *widget)
{
	gtk_widget_unparent (widget);
}

void
pps_element_widget_factory_setup (PpsElementWidgetFactory *factory,
                                  PpsDocumentModel *model,
                                  PpsAnnotationsContext *annots_context,
                                  PpsPixbufCache *pixbuf_cache,
                                  GPtrArray *page_widgets,
                                  PpsPageCache *page_cache)
{
	PpsElementWidgetFactoryPrivate *priv = GET_PRIVATE (factory);

	priv->page_widgets = page_widgets;

	for (gint i = 0; i < page_widgets->len; i++) {
		g_signal_connect (g_ptr_array_index (priv->page_widgets, i),
		                  "notify::page", G_CALLBACK (on_page_changed), factory);
	}

	if (priv->model != NULL)
		g_signal_handlers_disconnect_by_data (priv->model, factory);
	if (priv->pixbuf_cache != NULL)
		g_signal_handlers_disconnect_by_data (priv->pixbuf_cache, factory);

	g_set_object (&priv->model, model);
	g_set_object (&priv->pixbuf_cache, pixbuf_cache);

	g_signal_connect (priv->pixbuf_cache, "job-finished",
	                  G_CALLBACK (job_finished_cb), factory);

	PPS_ELEMENT_WIDGET_FACTORY_GET_CLASS (factory)->setup (factory,
	                                                       model,
	                                                       annots_context,
	                                                       pixbuf_cache,
	                                                       page_widgets,
	                                                       page_cache);
}

static void
pps_element_widget_factory_class_init (PpsElementWidgetFactoryClass *page_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (page_class);

	object_class->dispose = pps_element_widget_factory_dispose;
}

PpsElementWidgetFactory *
pps_element_widget_factory_new (void)
{
	return g_object_new (PPS_TYPE_ELEMENT_WIDGET_FACTORY, NULL);
}
