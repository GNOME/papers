// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.h"

#include "pps-snapshot-utils.h"

#include <math.h>

static void
append_page_texture (GtkSnapshot *snapshot,
                     GdkTexture *texture,
                     const graphene_rect_t *area)
{
	graphene_rect_t dest = *area;
	GskScalingFilter filter;
	int tw = gdk_texture_get_width (texture);
	int th = gdk_texture_get_height (texture);

	/* After gtk_snapshot_scale (1/device_scale), @area is in physical
	 * pixels. Never NEAREST: a 1px size/origin miss (typical at 150%)
	 * turns Type 1 into a 1-bit fax. TRILINEAR at 1:1 is a copy;
	 * under zoom it stays readable. */
	if (fabsf (dest.size.width - tw) < 0.51f &&
	    fabsf (dest.size.height - th) < 0.51f) {
		dest.size.width = tw;
		dest.size.height = th;
	}
	filter = GSK_SCALING_FILTER_TRILINEAR;

	gtk_snapshot_append_scaled_texture (snapshot, texture, filter, &dest);
}

void
pps_snapshot_append_page_texture (GtkSnapshot *snapshot,
                                  GdkTexture *texture,
                                  const graphene_rect_t *area,
                                  gboolean inverted)
{
	g_return_if_fail (snapshot != NULL);
	g_return_if_fail (GDK_IS_TEXTURE (texture));
	g_return_if_fail (area != NULL);

	gtk_snapshot_save (snapshot);

	if (inverted) {
		gtk_snapshot_push_blend (snapshot, GSK_BLEND_MODE_COLOR);
		gtk_snapshot_push_blend (snapshot, GSK_BLEND_MODE_DIFFERENCE);
		gtk_snapshot_append_color (snapshot, &(GdkRGBA) { 1., 1., 1., 1. }, area);
		gtk_snapshot_pop (snapshot);
		append_page_texture (snapshot, texture, area);
		gtk_snapshot_pop (snapshot);
		gtk_snapshot_pop (snapshot);
		append_page_texture (snapshot, texture, area);
		gtk_snapshot_pop (snapshot);
	} else {
		append_page_texture (snapshot, texture, area);
	}

	gtk_snapshot_restore (snapshot);
}
