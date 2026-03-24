// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-css-utils.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */
#pragma once

#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include "pps-macros.h"
#include <pango/pango.h>

G_BEGIN_DECLS

gchar *dzl_pango_font_description_to_css (const PangoFontDescription *font_desc);

PPS_PUBLIC
char *pps_css_utils_filter_class_chars (const char *str);

G_END_DECLS
