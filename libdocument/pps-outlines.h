// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2024 Qiu Wenbo
 */
#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>
#include <papers-document.h>

G_BEGIN_DECLS

#define PPS_TYPE_OUTLINES (pps_outlines_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsOutlines, pps_outlines, PPS, OUTLINES, GObject);

struct _PpsOutlines {
	GObject parent;
};

PPS_PUBLIC
PpsOutlines *pps_outlines_new (void);

PPS_PUBLIC
void pps_outlines_set_markup (PpsOutlines *pps_outlines, const gchar *markup);
PPS_PUBLIC
void pps_outlines_set_label (PpsOutlines *pps_outlines, const gchar *label);
PPS_PUBLIC
void pps_outlines_set_expand (PpsOutlines *pps_outlines, gboolean expand);
PPS_PUBLIC
void pps_outlines_set_link (PpsOutlines *pps_outlines, PpsLink *link);
PPS_PUBLIC
void pps_outlines_set_children (PpsOutlines *pps_outlines, GListModel *children);

PPS_PUBLIC
PpsLink *pps_outlines_get_link (PpsOutlines *pps_outlines);
PPS_PUBLIC
GListModel *pps_outlines_get_children (PpsOutlines *pps_outlines);
PPS_PUBLIC
gboolean pps_outlines_get_expand (PpsOutlines *pps_outlines);

G_END_DECLS
