/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2013 Carlos Garcia Campos <carlosgc@gnome.org>
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
