/*
 *  Copyright (C) 2005 Red Hat, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
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
