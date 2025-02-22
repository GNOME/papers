/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2014 Igalia S.L.
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
 *
 * Author: Alejandro Piñeiro Iglesias <apinheiro@igalia.com>
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include "pps-view-accessible.h"
#include "pps-view.h"

#define PPS_TYPE_PAGE_ACCESSIBLE (pps_page_accessible_get_type ())
#define PPS_PAGE_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_PAGE_ACCESSIBLE, PpsPageAccessible))
#define PPS_IS_PAGE_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_PAGE_ACCESSIBLE))

typedef struct _PpsPageAccessible PpsPageAccessible;
typedef struct _PpsPageAccessibleClass PpsPageAccessibleClass;
typedef struct _PpsPageAccessiblePrivate PpsPageAccessiblePrivate;

struct _PpsPageAccessible {
	AtkObject parent;

	PpsPageAccessiblePrivate *priv;
};

struct _PpsPageAccessibleClass {
	AtkObjectClass parent_class;
};

GType pps_page_accessible_get_type (void) G_GNUC_CONST;
PpsPageAccessible *pps_page_accessible_new (PpsViewAccessible *view_accessible,
                                            gint page);
gint pps_page_accessible_get_page (PpsPageAccessible *page_accessible);
PpsViewAccessible *pps_page_accessible_get_view_accessible (PpsPageAccessible *page_accessible);
PpsView *pps_page_accessible_get_view (PpsPageAccessible *page_accessible);
AtkObject *pps_page_accessible_get_accessible_for_mapping (PpsPageAccessible *page_accessible,
                                                           PpsMapping *mapping);
void pps_page_accessible_update_element_state (PpsPageAccessible *page_accessible,
                                               PpsMapping *mapping);
