// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2006 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>

#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_IMAGE (pps_image_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsImage, pps_image, PPS, IMAGE, GObject)

struct _PpsImage {
	GObject base_instance;
};

PPS_PUBLIC
PpsImage *pps_image_new (gint page,
                         gint img_id);
PPS_PUBLIC
PpsImage *pps_image_new_from_pixbuf (GdkPixbuf *pixbuf);

PPS_PUBLIC
gint pps_image_get_id (PpsImage *image);
PPS_PUBLIC
gint pps_image_get_page (PpsImage *image);
PPS_PUBLIC
GdkPixbuf *pps_image_get_pixbuf (PpsImage *image);
PPS_PUBLIC
const gchar *pps_image_save_tmp (PpsImage *image,
                                 GdkPixbuf *pixbuf);
PPS_PUBLIC
const gchar *pps_image_get_tmp_uri (PpsImage *image);

G_END_DECLS
