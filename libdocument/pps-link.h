// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2005 Red Hat, Inc.
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-document.h"
#include "pps-link-action.h"
#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_LINK (pps_link_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsLink, pps_link, PPS, LINK, GObject)

PPS_PUBLIC
PpsLink *pps_link_new (const gchar *title,
                       PpsLinkAction *action);

PPS_PUBLIC
const gchar *pps_link_get_title (PpsLink *self);
PPS_PUBLIC
PpsLinkAction *pps_link_get_action (PpsLink *self);

G_END_DECLS
