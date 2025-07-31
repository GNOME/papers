// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2005 Red Hat, Inc.
 */

#include "config.h"

#include "pps-selection.h"

G_DEFINE_INTERFACE (PpsSelection, pps_selection, 0)

static void
pps_selection_default_init (PpsSelectionInterface *klass)
{
}

void
pps_selection_render_selection (PpsSelection *selection,
                                PpsRenderContext *rc,
                                cairo_surface_t **surface,
                                PpsRectangle *points,
                                PpsRectangle *old_points,
                                PpsSelectionStyle style,
                                GdkRGBA *text,
                                GdkRGBA *base)
{
	PpsSelectionInterface *iface = PPS_SELECTION_GET_IFACE (selection);

	if (!iface->render_selection)
		return;

	iface->render_selection (selection, rc,
	                         surface,
	                         points, old_points,
	                         style,
	                         text, base);
}

gchar *
pps_selection_get_selected_text (PpsSelection *selection,
                                 PpsPage *page,
                                 PpsSelectionStyle style,
                                 PpsRectangle *points)
{
	PpsSelectionInterface *iface = PPS_SELECTION_GET_IFACE (selection);

	return iface->get_selected_text (selection, page, style, points);
}

cairo_region_t *
pps_selection_get_selection_region (PpsSelection *selection,
                                    PpsRenderContext *rc,
                                    PpsSelectionStyle style,
                                    PpsRectangle *points)
{
	PpsSelectionInterface *iface = PPS_SELECTION_GET_IFACE (selection);

	if (!iface->get_selection_region)
		return NULL;

	return iface->get_selection_region (selection, rc, style, points);
}
