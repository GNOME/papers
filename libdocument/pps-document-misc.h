/*
 *  Copyright (C) 2000-2003 Marco Pesenti Gritti
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <cairo.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "pps-macros.h"

G_BEGIN_DECLS

PPS_PUBLIC
cairo_surface_t *pps_document_misc_surface_from_pixbuf (GdkPixbuf *pixbuf);
PPS_PUBLIC
GdkPixbuf *pps_document_misc_pixbuf_from_surface (cairo_surface_t *surface);
PPS_PUBLIC
GdkTexture *pps_document_misc_texture_from_surface (cairo_surface_t *surface);
PPS_PUBLIC
cairo_surface_t *pps_document_misc_surface_rotate_and_scale (cairo_surface_t *surface,
                                                             gint dest_width,
                                                             gint dest_height,
                                                             gint dest_rotation);

PPS_PUBLIC
gdouble pps_document_misc_get_widget_dpi (GtkWidget *widget);

PPS_PUBLIC
gchar *pps_document_misc_format_datetime (GDateTime *dt);

PPS_PUBLIC
gboolean pps_document_misc_get_pointer_position (GtkWidget *widget,
                                                 gint *x,
                                                 gint *y);

G_END_DECLS
