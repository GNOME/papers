// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-form-overlay.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#pragma once

#include <gtk/gtk.h>

#include "pps-document-model.h"
#include "pps-page-cache.h"
#include "pps-pixbuf-cache.h"

G_BEGIN_DECLS

#define PPS_TYPE_OVERLAY_FORM (pps_overlay_form_get_type ())
G_DECLARE_DERIVABLE_TYPE (PpsOverlayForm, pps_overlay_form, PPS, OVERLAY_FORM, GtkBox)

struct _PpsOverlayFormClass {
	GtkBoxClass base_class;
};

GtkWidget *pps_overlay_form_new (PpsFormField *field,
                                 PpsDocumentModel *model,
                                 PpsPageCache *page_cache,
                                 PpsPixbufCache *pixbuf_cache);

G_END_DECLS
