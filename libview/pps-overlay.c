// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-overlay.c
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#include "pps-overlay.h"

G_DEFINE_INTERFACE (PpsOverlay, pps_overlay, G_TYPE_OBJECT)

static void
pps_overlay_default_init (PpsOverlayInterface *iface)
{
	iface->get_area = NULL;
	iface->update_visibility_from_state = NULL;
}

/**
 * pps_overlay_get_area:
 * @self: a #PpsOverlay instance
 * @padding: (out): an additional padding for the overlay area (in view units)
 *
 * Gets the document area of the overlay.
 *
 * Returns: (transfer full): a #PpsRectangle that describes where the overlay shall be positioned
 */
PpsRectangle *
pps_overlay_get_area (PpsOverlay *self, gdouble *padding)
{
	g_return_val_if_fail (PPS_IS_OVERLAY (self), NULL);
	PpsOverlayInterface *iface = PPS_OVERLAY_GET_IFACE (self);
	g_return_val_if_fail (iface->get_area != NULL, NULL);
	return iface->get_area (self, padding);
}

void
pps_overlay_update_visibility_from_state (PpsOverlay *self, PpsRenderAnnotsFlags state)
{
	g_return_if_fail (PPS_IS_OVERLAY (self));
	PpsOverlayInterface *iface = PPS_OVERLAY_GET_IFACE (self);
	if (iface->update_visibility_from_state) {
		iface->update_visibility_from_state (self, state);
	} else {
		gtk_widget_set_visible (GTK_WIDGET (self), TRUE);
	}
}
