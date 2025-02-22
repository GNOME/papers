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

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include "pps-image.h"
#include "pps-page-accessible.h"

#define PPS_TYPE_IMAGE_ACCESSIBLE (pps_image_accessible_get_type ())
#define PPS_IMAGE_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_IMAGE_ACCESSIBLE, PpsImageAccessible))
#define PPS_IS_IMAGE_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_IMAGE_ACCESSIBLE))

typedef struct _PpsImageAccessible PpsImageAccessible;
typedef struct _PpsImageAccessibleClass PpsImageAccessibleClass;
typedef struct _PpsImageAccessiblePrivate PpsImageAccessiblePrivate;

struct _PpsImageAccessible {
	AtkObject parent;
	PpsImageAccessiblePrivate *priv;
};

struct _PpsImageAccessibleClass {
	AtkObjectClass parent_class;
};

GType pps_image_accessible_get_type (void);
PpsImageAccessible *pps_image_accessible_new (PpsPageAccessible *page,
                                              PpsImage *image,
                                              PpsRectangle *area);
