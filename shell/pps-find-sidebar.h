/* pps-find-sidebar.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2013 Carlos Garcia Campos  <carlosgc@gnome.org>
 * Copyright (C) 2008 Sergey Pushkin  <pushkinsv@gmail.com >
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

#include <adwaita.h>
#include <gtk/gtk.h>

#include "pps-jobs.h"
#include <papers-view.h>

G_BEGIN_DECLS

#define PPS_TYPE_FIND_SIDEBAR (pps_find_sidebar_get_type ())
#define PPS_FIND_SIDEBAR(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), PPS_TYPE_FIND_SIDEBAR, PpsFindSidebar))
#define PPS_IS_FIND_SIDEBAR(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PPS_TYPE_FIND_SIDEBAR))
#define PPS_FIND_SIDEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_FIND_SIDEBAR, PpsFindSidebarClass))
#define PPS_IS_FIND_SIDEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_FIND_SIDEBAR))
#define PPS_FIND_SIDEBAR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), PPS_TYPE_FIND_SIDEBAR, PpsFindSidebarClass))

typedef struct _PpsFindSidebar PpsFindSidebar;
typedef struct _PpsFindSidebarClass PpsFindSidebarClass;

struct _PpsFindSidebar {
	AdwBin base_instance;
};

struct _PpsFindSidebarClass {
	AdwBinClass base_class;
};

GType pps_find_sidebar_get_type (void);
GtkWidget *pps_find_sidebar_new (void);

void pps_find_sidebar_update (PpsFindSidebar *find_sidebar);
void pps_find_sidebar_clear (PpsFindSidebar *find_sidebar);
void pps_find_sidebar_previous (PpsFindSidebar *find_sidebar);
void pps_find_sidebar_next (PpsFindSidebar *find_sidebar);
void pps_find_sidebar_set_search_context (PpsFindSidebar *find_sidebar,
                                          PpsSearchContext *context);

G_END_DECLS
