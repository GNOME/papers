// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2014 Igalia S.L.
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
