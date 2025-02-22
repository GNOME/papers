/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2014 Igalia
 * Author: Joanmarie Diggs <jdiggs@igalia.com>
 *
 * Papers is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Papers is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include "pps-image-accessible.h"
#include "pps-view-private.h"

struct _PpsImageAccessiblePrivate {
	PpsPageAccessible *page;
	PpsImage *image;
	PpsRectangle area;
};

static void pps_image_accessible_component_iface_init (AtkComponentIface *iface);
static void pps_image_accessible_image_iface_init (AtkImageIface *iface);

G_DEFINE_TYPE_WITH_CODE (PpsImageAccessible, pps_image_accessible, ATK_TYPE_OBJECT, G_ADD_PRIVATE (PpsImageAccessible) G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, pps_image_accessible_component_iface_init) G_IMPLEMENT_INTERFACE (ATK_TYPE_IMAGE, pps_image_accessible_image_iface_init))

static void
pps_image_accessible_get_atk_rect (AtkObject *atk_object,
                                   AtkCoordType coord_type,
                                   PpsRectangle *atk_rect)
{
	PpsImageAccessible *image;
	PpsViewAccessible *view_accessible;
	gint page;

	image = PPS_IMAGE_ACCESSIBLE (atk_object);
	view_accessible = pps_page_accessible_get_view_accessible (image->priv->page);
	page = pps_page_accessible_get_page (image->priv->page);
	_transform_doc_rect_to_atk_rect (view_accessible, page, &image->priv->area, atk_rect, coord_type);
}

static void
pps_image_accessible_get_extents (AtkComponent *atk_component,
                                  gint *x,
                                  gint *y,
                                  gint *width,
                                  gint *height,
                                  AtkCoordType coord_type)
{
	PpsRectangle atk_rect;

	pps_image_accessible_get_atk_rect (ATK_OBJECT (atk_component), coord_type, &atk_rect);
	*x = atk_rect.x1;
	*y = atk_rect.y1;
	*width = atk_rect.x2 - atk_rect.x1;
	*height = atk_rect.y2 - atk_rect.y1;
}

static void
pps_image_accessible_component_iface_init (AtkComponentIface *iface)
{
	iface->get_extents = pps_image_accessible_get_extents;
}

static const gchar *
pps_image_accessible_get_image_description (AtkImage *image)
{
	/* This will be obtainable for tagged PDFs */
	return NULL;
}

static const gchar *
pps_image_accessible_get_image_locale (AtkImage *atk_image)
{
	/* This will be obtainable for tagged PDFs */
	return NULL;
}

static void
pps_image_accessible_get_image_size (AtkImage *atk_image,
                                     int *width,
                                     int *height)
{
	PpsRectangle atk_rect;

	pps_image_accessible_get_atk_rect (ATK_OBJECT (atk_image), ATK_XY_WINDOW, &atk_rect);
	*width = atk_rect.x2 - atk_rect.x1;
	*height = atk_rect.y2 - atk_rect.y1;
}

static void
pps_image_accessible_get_image_position (AtkImage *atk_image,
                                         gint *x,
                                         gint *y,
                                         AtkCoordType coord_type)
{
	PpsRectangle atk_rect;

	pps_image_accessible_get_atk_rect (ATK_OBJECT (atk_image), ATK_XY_WINDOW, &atk_rect);
	*x = atk_rect.x1;
	*y = atk_rect.y1;
}

static AtkStateSet *
pps_image_accessible_ref_state_set (AtkObject *atk_object)
{
	AtkStateSet *state_set;
	AtkStateSet *copy_set;
	AtkStateSet *page_accessible_state_set;
	PpsImageAccessible *self;
	PpsViewAccessible *view_accessible;
	gint page;

	self = PPS_IMAGE_ACCESSIBLE (atk_object);
	state_set = ATK_OBJECT_CLASS (pps_image_accessible_parent_class)->ref_state_set (atk_object);
	atk_state_set_clear_states (state_set);

	page_accessible_state_set = atk_object_ref_state_set (ATK_OBJECT (self->priv->page));
	copy_set = atk_state_set_or_sets (state_set, page_accessible_state_set);
	atk_state_set_remove_state (copy_set, ATK_STATE_FOCUSABLE);
	atk_state_set_remove_state (copy_set, ATK_STATE_FOCUSED);

	view_accessible = pps_page_accessible_get_view_accessible (self->priv->page);
	page = pps_page_accessible_get_page (self->priv->page);
	if (!pps_view_accessible_is_doc_rect_showing (view_accessible, page, &self->priv->area))
		atk_state_set_remove_state (copy_set, ATK_STATE_SHOWING);

	g_object_unref (state_set);
	g_object_unref (page_accessible_state_set);

	return copy_set;
}

static AtkObject *
pps_image_accessible_get_parent (AtkObject *atk_object)
{
	PpsImageAccessiblePrivate *priv = PPS_IMAGE_ACCESSIBLE (atk_object)->priv;

	return ATK_OBJECT (priv->page);
}

static void
pps_image_accessible_finalize (GObject *object)
{
	PpsImageAccessiblePrivate *priv = PPS_IMAGE_ACCESSIBLE (object)->priv;

	g_object_unref (priv->image);

	G_OBJECT_CLASS (pps_image_accessible_parent_class)->finalize (object);
}

static void
pps_image_accessible_class_init (PpsImageAccessibleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

	object_class->finalize = pps_image_accessible_finalize;
	atk_class->get_parent = pps_image_accessible_get_parent;
	atk_class->ref_state_set = pps_image_accessible_ref_state_set;
}

static void
pps_image_accessible_init (PpsImageAccessible *image)
{
	atk_object_set_role (ATK_OBJECT (image), ATK_ROLE_IMAGE);

	image->priv = pps_image_accessible_get_instance_private (image);
}

static void
pps_image_accessible_image_iface_init (AtkImageIface *iface)
{
	iface->get_image_description = pps_image_accessible_get_image_description;
	iface->get_image_locale = pps_image_accessible_get_image_locale;
	iface->get_image_position = pps_image_accessible_get_image_position;
	iface->get_image_size = pps_image_accessible_get_image_size;
}

PpsImageAccessible *
pps_image_accessible_new (PpsPageAccessible *page,
                          PpsImage *image,
                          PpsRectangle *area)
{
	PpsImageAccessible *atk_image;

	atk_image = g_object_new (PPS_TYPE_IMAGE_ACCESSIBLE, NULL);
	atk_image->priv->page = page;
	atk_image->priv->image = g_object_ref (image);
	atk_image->priv->area = *area;

	return PPS_IMAGE_ACCESSIBLE (atk_image);
}
