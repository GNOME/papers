// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2014 Igalia
 * Author: Joanmarie Diggs <jdiggs@igalia.com>
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
