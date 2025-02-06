// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-css-utils.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Lucas Baudin <lbaudin@gnome.org>
 */

#include <pango/pango.h>

gchar *
dzl_pango_font_description_to_css (const PangoFontDescription *font_desc);
