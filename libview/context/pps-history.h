// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2005 Marco Pesenti Gritti

 */

#pragma once

#include <libdocument/pps-macros.h>
#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include "pps-document-model.h"
#include <glib-object.h>
#include <papers-document.h>

G_BEGIN_DECLS

PPS_PUBLIC
#define PPS_TYPE_HISTORY (pps_history_get_type ())

G_DECLARE_DERIVABLE_TYPE (PpsHistory, pps_history, PPS, HISTORY, GObject)

struct _PpsHistoryClass {
	GObjectClass parent_class;

	void (*changed) (PpsHistory *history);
	void (*activate_link) (PpsHistory *history,
	                       PpsLink *link);
};

PPS_PUBLIC
PpsHistory *pps_history_new (PpsDocumentModel *model);
PPS_PUBLIC
void pps_history_add_link (PpsHistory *history,
                           PpsLink *link);
PPS_PUBLIC
void pps_history_add_page (PpsHistory *history,
                           gint page);
PPS_PUBLIC
gboolean pps_history_can_go_back (PpsHistory *history);
PPS_PUBLIC
void pps_history_go_back (PpsHistory *history);
PPS_PUBLIC
gboolean pps_history_can_go_forward (PpsHistory *history);
PPS_PUBLIC
void pps_history_go_forward (PpsHistory *history);
PPS_PUBLIC
gboolean pps_history_go_to_link (PpsHistory *history,
                                 PpsLink *link);
GList *pps_history_get_back_list (PpsHistory *history);
PPS_PUBLIC
GList *pps_history_get_forward_list (PpsHistory *history);

PPS_PUBLIC
void pps_history_freeze (PpsHistory *history);
PPS_PUBLIC
void pps_history_thaw (PpsHistory *history);
PPS_PUBLIC
gboolean pps_history_is_frozen (PpsHistory *history);

G_END_DECLS
