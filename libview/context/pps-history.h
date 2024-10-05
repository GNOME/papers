/*
 *  Copyright (C) 2005 Marco Pesenti Gritti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA..
 *
 */

#pragma once

#include <libdocument/pps-macros.h>
#if !defined (__PPS_PAPERS_VIEW_H_INSIDE__) && !defined (PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <glib-object.h>
#include <papers-document.h>
#include "pps-document-model.h"

G_BEGIN_DECLS

PPS_PUBLIC
#define PPS_TYPE_HISTORY            (pps_history_get_type ())

G_DECLARE_DERIVABLE_TYPE (PpsHistory, pps_history, PPS, HISTORY, GObject)

struct _PpsHistoryClass
{
	GObjectClass parent_class;

	void (* changed)       (PpsHistory *history);
        void (* activate_link) (PpsHistory *history,
                                PpsLink    *link);
};

PPS_PUBLIC
PpsHistory      *pps_history_new              (PpsDocumentModel *model);
PPS_PUBLIC
void            pps_history_add_link         (PpsHistory       *history,
                                             PpsLink          *link);
PPS_PUBLIC
void            pps_history_add_page         (PpsHistory       *history,
                                             gint            page);
PPS_PUBLIC
gboolean        pps_history_can_go_back      (PpsHistory       *history);
PPS_PUBLIC
void            pps_history_go_back          (PpsHistory       *history);
PPS_PUBLIC
gboolean        pps_history_can_go_forward   (PpsHistory       *history);
PPS_PUBLIC
void            pps_history_go_forward       (PpsHistory       *history);
PPS_PUBLIC
gboolean        pps_history_go_to_link       (PpsHistory       *history,
                                             PpsLink          *link);
GList          *pps_history_get_back_list    (PpsHistory       *history);
PPS_PUBLIC
GList          *pps_history_get_forward_list (PpsHistory       *history);

PPS_PUBLIC
void            pps_history_freeze           (PpsHistory       *history);
PPS_PUBLIC
void            pps_history_thaw             (PpsHistory       *history);
PPS_PUBLIC
gboolean        pps_history_is_frozen        (PpsHistory       *history);

G_END_DECLS
