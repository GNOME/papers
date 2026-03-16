// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation-widget-factory.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#pragma once

#include "context/pps-annotations-context.h"
#include "context/pps-document-model.h"
#include "factory/pps-element-widget-factory.h"

G_BEGIN_DECLS

#define PPS_TYPE_ANNOTATION_WIDGET_FACTORY (pps_annotation_widget_factory_get_type ())

G_DECLARE_FINAL_TYPE (PpsAnnotationWidgetFactory, pps_annotation_widget_factory, PPS, ANNOTATION_WIDGET_FACTORY, PpsElementWidgetFactory)

struct _PpsAnnotationWidgetFactory {
	PpsElementWidgetFactory parent_instance;
};

struct _PpsAnnotationWidgetFactoryClass {
	PpsElementWidgetFactoryClass parent_class;
};

PpsElementWidgetFactory *pps_annotation_widget_factory_new (void);

GtkWidget *pps_annotation_widget_factory_get_widget_for_annot (PpsAnnotationWidgetFactory *factory,
                                                               PpsAnnotation *annot);

G_END_DECLS
