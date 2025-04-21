// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2013 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include "pps-link.h"
#include "pps-page-accessible.h"

#define PPS_TYPE_LINK_ACCESSIBLE (pps_link_accessible_get_type ())
#define PPS_LINK_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_LINK_ACCESSIBLE, PpsLinkAccessible))
#define PPS_IS_LINK_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_LINK_ACCESSIBLE))

typedef struct _PpsLinkAccessible PpsLinkAccessible;
typedef struct _PpsLinkAccessibleClass PpsLinkAccessibleClass;
typedef struct _PpsLinkAccessiblePrivate PpsLinkAccessiblePrivate;

struct _PpsLinkAccessible {
	AtkObject parent;

	PpsLinkAccessiblePrivate *priv;
};

struct _PpsLinkAccessibleClass {
	AtkObjectClass parent_class;
};

GType pps_link_accessible_get_type (void);
PpsLinkAccessible *pps_link_accessible_new (PpsPageAccessible *page,
                                            PpsLink *link,
                                            PpsRectangle *area);
