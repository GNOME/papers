// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2000-2003 Marco Pesenti Gritti
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
