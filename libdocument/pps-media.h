// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2015 Igalia S.L.
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-macros.h"
#include "pps-page.h"

G_BEGIN_DECLS

#define PPS_TYPE_MEDIA (pps_media_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsMedia, pps_media, PPS, MEDIA, GObject)

struct _PpsMedia {
	GObject base_instance;
};

PPS_PUBLIC
PpsMedia *pps_media_new_for_uri (PpsPage *page,
                                 const gchar *uri);
PPS_PUBLIC
const gchar *pps_media_get_uri (PpsMedia *media);
PPS_PUBLIC
guint pps_media_get_page_index (PpsMedia *media);
PPS_PUBLIC
gboolean pps_media_get_show_controls (PpsMedia *media);
PPS_PUBLIC
void pps_media_set_show_controls (PpsMedia *media,
                                  gboolean show_controls);

G_END_DECLS
