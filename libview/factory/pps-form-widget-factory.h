// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-form-widget-factory.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#pragma once

#include "context/pps-annotations-context.h"
#include "context/pps-document-model.h"
#include "factory/pps-element-widget-factory.h"

G_BEGIN_DECLS

#define PPS_TYPE_FORM_WIDGET_FACTORY (pps_form_widget_factory_get_type ())

G_DECLARE_FINAL_TYPE (PpsFormWidgetFactory, pps_form_widget_factory, PPS, FORM_WIDGET_FACTORY, PpsElementWidgetFactory)

struct _PpsFormWidgetFactory {
	PpsElementWidgetFactory parent_instance;
};

struct _PpsFormWidgetFactoryClass {
	PpsElementWidgetFactoryClass parent_class;
};

PpsElementWidgetFactory *pps_form_widget_factory_new (void);

G_END_DECLS
