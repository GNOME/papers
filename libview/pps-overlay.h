// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-overlay.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#pragma once

#include "pps-document-model.h"
#include "pps-document.h"
#include <gtk/gtk.h>

#define PPS_TYPE_OVERLAY (pps_overlay_get_type ())
G_DECLARE_INTERFACE (PpsOverlay, pps_overlay, PPS, OVERLAY, GObject)

struct _PpsOverlayInterface {
	GTypeInterface parent_iface;

	PpsRectangle *(*get_area) (PpsOverlay *self, gdouble *padding);
	void (*update_visibility_from_state) (PpsOverlay *self, PpsRenderAnnotsFlags state);
};

PpsRectangle *pps_overlay_get_area (PpsOverlay *self, gdouble *padding);
void pps_overlay_update_visibility_from_state (PpsOverlay *self, PpsRenderAnnotsFlags state);
