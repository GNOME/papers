// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <gio/gio.h>
#include <glib-object.h>

#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_LAYER (pps_layer_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsLayer, pps_layer, PPS, LAYER, GObject)

struct _PpsLayer {
	GObject base_instance;
};

PPS_PUBLIC
PpsLayer *pps_layer_new (gint rb_group);
PPS_PUBLIC
gint pps_layer_get_rb_group (PpsLayer *layer);
PPS_PUBLIC
void pps_layer_set_children (PpsLayer *pps_layer,
                             GListModel *children);
PPS_PUBLIC
GListModel *pps_layer_get_children (PpsLayer *pps_layer);

G_END_DECLS
