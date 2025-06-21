// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-form-widget-factory.c
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#include "pps-form-widget-factory.h"
#include "pps-form-overlay.h"

typedef struct
{
	PpsDocumentModel *model;
	GHashTable *form_widgets;
	PpsPageCache *page_cache;
	PpsPixbufCache *pixbuf_cache;
	GtkCssProvider *provider;
} PpsFormWidgetFactoryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsFormWidgetFactory, pps_form_widget_factory, PPS_TYPE_ELEMENT_WIDGET_FACTORY)

#define GET_PRIVATE(o) pps_form_widget_factory_get_instance_private (o)

static void
pps_form_widget_factory_init (PpsFormWidgetFactory *factory)
{
	PpsFormWidgetFactoryPrivate *priv = GET_PRIVATE (factory);
	priv->form_widgets = g_hash_table_new (NULL, NULL);
	priv->provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_string (priv->provider,
	                                   ".overlay-form {"
	                                   "  opacity: 0;"
	                                   "}"
	                                   ".overlay-form:focus-within {"
	                                   "  opacity: 1.0;"
	                                   "}"
	                                   ".overlay-form checkbutton > * {"
	                                   "  opacity: 0;"
	                                   "}"
	                                   ".overlay-form checkbutton {"
	                                   "  background: none;"
	                                   "}");
	gtk_style_context_add_provider_for_display (gdk_display_get_default (),
	                                            GTK_STYLE_PROVIDER (priv->provider),
	                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static void
pps_form_widget_factory_dispose (GObject *object)
{
	PpsFormWidgetFactory *factory = PPS_FORM_WIDGET_FACTORY (object);
	PpsFormWidgetFactoryPrivate *priv = GET_PRIVATE (factory);

	g_clear_object (&priv->model);
	g_clear_object (&priv->page_cache);
	g_clear_object (&priv->provider);

	g_hash_table_destroy (priv->form_widgets);

	G_OBJECT_CLASS (pps_form_widget_factory_parent_class)->dispose (object);
}

static GList *
pps_form_widget_factory_widgets_for_page (PpsElementWidgetFactory *factory, guint page_index)
{
	PpsFormWidgetFactory *annot_factory = PPS_FORM_WIDGET_FACTORY (factory);
	PpsFormWidgetFactoryPrivate *priv = GET_PRIVATE (annot_factory);
	GList *widgets = NULL;

	GList *mappings = pps_mapping_list_get_list (pps_page_cache_get_form_field_mapping (priv->page_cache, page_index));
	while (mappings) {
		PpsMapping *mapping = mappings->data;
		PpsFormField *field = PPS_FORM_FIELD (mapping->data);
		GtkWidget *form = g_hash_table_lookup (priv->form_widgets, field);
		if (!form) {
			form = pps_overlay_form_new (field,
			                             priv->model,
			                             priv->page_cache,
			                             priv->pixbuf_cache);
			g_hash_table_insert (priv->form_widgets, field, g_object_ref_sink (form));
		}
		widgets = g_list_append (widgets, form);
		mappings = mappings->next;
	}

	return widgets;
}

static void
on_page_changed (PpsPageCache *page_cache, gint page, PpsElementWidgetFactory *factory)
{
	pps_element_widget_factory_query_reload (factory);
}

static void
pps_form_widget_factory_setup (PpsElementWidgetFactory *element_factory,
                               PpsDocumentModel *model,
                               PpsAnnotationsContext *annots_context,
                               PpsPixbufCache *pixbuf_cache,
                               GPtrArray *page_widgets,
                               PpsPageCache *page_cache)
{
	PpsFormWidgetFactory *factory = PPS_FORM_WIDGET_FACTORY (element_factory);
	PpsFormWidgetFactoryPrivate *priv = GET_PRIVATE (factory);

	if (priv->model != NULL)
		g_signal_handlers_disconnect_by_data (priv->model, factory);

	if (priv->page_cache != NULL)
		g_signal_handlers_disconnect_by_data (priv->page_cache, factory);

	g_set_object (&priv->model, model);
	g_set_object (&priv->page_cache, page_cache);
	g_set_object (&priv->pixbuf_cache, pixbuf_cache);
	g_signal_connect (priv->page_cache, "page-cached", G_CALLBACK (on_page_changed), element_factory);
}

static gboolean
pps_form_widget_factory_is_managed (PpsElementWidgetFactory *factory, GtkWidget *widget)
{
	return PPS_IS_OVERLAY_FORM (widget);
}

static void
pps_form_widget_factory_class_init (PpsFormWidgetFactoryClass *page_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (page_class);
	PpsElementWidgetFactoryClass *factory_class = PPS_ELEMENT_WIDGET_FACTORY_CLASS (page_class);

	object_class->dispose = pps_form_widget_factory_dispose;

	factory_class->widgets_for_page = pps_form_widget_factory_widgets_for_page;
	factory_class->setup = pps_form_widget_factory_setup;
	factory_class->is_managed = pps_form_widget_factory_is_managed;
}

PpsElementWidgetFactory *
pps_form_widget_factory_new (void)
{
	return g_object_new (PPS_TYPE_FORM_WIDGET_FACTORY, NULL);
}
