// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-element-widget-factory.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#pragma once

#include <papers-document.h>
#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include "context/pps-annotations-context.h"
#include "context/pps-document-model.h"
#include "pps-pixbuf-cache.h"
#include "pps-view-page.h"

G_BEGIN_DECLS

#define PPS_TYPE_ELEMENT_WIDGET_FACTORY (pps_element_widget_factory_get_type ())

G_DECLARE_DERIVABLE_TYPE (PpsElementWidgetFactory, pps_element_widget_factory, PPS, ELEMENT_WIDGET_FACTORY, GObject)

struct _PpsElementWidgetFactoryClass {
	GObjectClass parent_class;

	GList *(*widgets_for_page) (PpsElementWidgetFactory *factory, guint page_index);

	gboolean (*is_managed) (PpsElementWidgetFactory *factory, GtkWidget *widget);

	void (*setup) (PpsElementWidgetFactory *factory,
	               PpsDocumentModel *model,
	               PpsAnnotationsContext *annot_context,
	               PpsPixbufCache *pixbuf_cache,
	               GPtrArray *page_widgets,
	               PpsPageCache *page_cache);
};

PpsElementWidgetFactory *pps_element_widget_factory_new (void);

void
pps_element_widget_factory_setup (PpsElementWidgetFactory *factory,
                                  PpsDocumentModel *model,
                                  PpsAnnotationsContext *annot_context,
                                  PpsPixbufCache *pixbuf_cache,
                                  GPtrArray *page_widgets,
                                  PpsPageCache *page_cache);

/* These three methods are used by descendants of the class and should not be used outside. */
void pps_element_widget_factory_query_reload (PpsElementWidgetFactory *factory);
void pps_element_widget_factory_new_widget_for_page (PpsElementWidgetFactory *factory, guint page_index, GtkWidget *widget);
void pps_element_widget_factory_widget_removed (PpsElementWidgetFactory *factory, guint page_index, GtkWidget *widget);

G_END_DECLS
