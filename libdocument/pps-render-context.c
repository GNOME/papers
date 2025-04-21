// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2005 Jonathan Blandford <jrb@gnome.org>
 */

#include "pps-render-context.h"
#include <config.h>

static void pps_render_context_init (PpsRenderContext *rc);
static void pps_render_context_class_init (PpsRenderContextClass *class);

G_DEFINE_TYPE (PpsRenderContext, pps_render_context, G_TYPE_OBJECT);

#define FLIP_DIMENSIONS(rc) ((rc)->rotation == 90 || (rc)->rotation == 270)

static void
pps_render_context_init (PpsRenderContext *rc)
{ /* Do Nothing */
}

static void
pps_render_context_dispose (GObject *object)
{
	PpsRenderContext *rc;

	rc = (PpsRenderContext *) object;

	g_clear_object (&rc->page);

	(*G_OBJECT_CLASS (pps_render_context_parent_class)->dispose) (object);
}

static void
pps_render_context_class_init (PpsRenderContextClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);

	oclass->dispose = pps_render_context_dispose;
}

PpsRenderContext *
pps_render_context_new (PpsPage *page,
                        gint rotation,
                        gdouble scale,
                        PpsRenderAnnotsFlags annot_flags)
{
	PpsRenderContext *rc;

	rc = (PpsRenderContext *) g_object_new (PPS_TYPE_RENDER_CONTEXT, NULL);

	rc->page = page ? g_object_ref (page) : NULL;
	rc->rotation = rotation;
	rc->scale = scale;
	rc->target_width = -1;
	rc->target_height = -1;
	rc->annot_flags = annot_flags;

	return rc;
}

void
pps_render_context_set_page (PpsRenderContext *rc,
                             PpsPage *page)
{
	g_return_if_fail (rc != NULL);
	g_return_if_fail (PPS_IS_PAGE (page));

	g_set_object (&rc->page, page);
}

void
pps_render_context_set_rotation (PpsRenderContext *rc,
                                 int rotation)
{
	g_return_if_fail (rc != NULL);

	rc->rotation = rotation;
}

void
pps_render_context_set_scale (PpsRenderContext *rc,
                              gdouble scale)
{
	g_return_if_fail (rc != NULL);

	rc->scale = scale;
}

void
pps_render_context_set_target_size (PpsRenderContext *rc,
                                    int target_width,
                                    int target_height)
{
	g_return_if_fail (rc != NULL);

	rc->target_width = target_width;
	rc->target_height = target_height;
}

/**
 * pps_render_context_compute_scaled_size:
 * @rc: an #PpsRenderContext
 * @width_points:
 * @height_points:
 * @scaled_width: (out):
 * @scaled_height: (out):
 *
 */
void
pps_render_context_compute_scaled_size (PpsRenderContext *rc,
                                        double width_points,
                                        double height_points,
                                        int *scaled_width,
                                        int *scaled_height)
{
	g_return_if_fail (rc != NULL);

	if (scaled_width) {
		if (rc->target_width >= 0) {
			*scaled_width = FLIP_DIMENSIONS (rc) ? rc->target_height : rc->target_width;
		} else {
			*scaled_width = (int) (width_points * rc->scale + 0.5);
		}
	}

	if (scaled_height) {
		if (rc->target_height >= 0) {
			*scaled_height = FLIP_DIMENSIONS (rc) ? rc->target_width : rc->target_height;
		} else {
			*scaled_height = (int) (height_points * rc->scale + 0.5);
		}
	}
}

/**
 * pps_render_context_compute_transformed_size:
 * @rc: an #PpsRenderContext
 * @width_points:
 * @height_points:
 * @transformed_width: (out):
 * @transformed_height: (out):
 *
 */
void
pps_render_context_compute_transformed_size (PpsRenderContext *rc,
                                             double width_points,
                                             double height_points,
                                             int *transformed_width,
                                             int *transformed_height)
{
	int scaled_width, scaled_height;

	g_return_if_fail (rc != NULL);

	pps_render_context_compute_scaled_size (rc, width_points, height_points,
	                                        &scaled_width, &scaled_height);

	if (transformed_width)
		*transformed_width = FLIP_DIMENSIONS (rc) ? scaled_height : scaled_width;

	if (transformed_height)
		*transformed_height = FLIP_DIMENSIONS (rc) ? scaled_width : scaled_height;
}

/**
 * pps_render_context_compute_scales:
 * @rc: an #PpsRenderContext
 * @width_points:
 * @height_points:
 * @scale_x: (out):
 * @scale_y: (out):
 *
 */
void
pps_render_context_compute_scales (PpsRenderContext *rc,
                                   double width_points,
                                   double height_points,
                                   double *scale_x,
                                   double *scale_y)
{
	int scaled_width, scaled_height;

	g_return_if_fail (rc != NULL);

	pps_render_context_compute_scaled_size (rc, width_points, height_points,
	                                        &scaled_width, &scaled_height);

	if (scale_x)
		*scale_x = scaled_width / width_points;

	if (scale_y)
		*scale_y = scaled_height / height_points;
}
