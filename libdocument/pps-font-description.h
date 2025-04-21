// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2024 Pablo Correa Gomez <ablocorrea@hotmail.com>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_FONT_DESCRIPTION (pps_font_description_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsFontDescription, pps_font_description, PPS, FONT_DESCRIPTION, GObject);

struct _PpsFontDescription {
	GObject parent;
};

PPS_PUBLIC
PpsFontDescription *pps_font_description_new (void);

G_END_DECLS
