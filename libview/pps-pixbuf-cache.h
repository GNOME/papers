/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2005 Red Hat, Inc
 *
 * Papers is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Papers is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/* This File is basically an extention of PpsView, and is out here just to keep
 * pps-view.c from exploding.
 */

#pragma once

#if !defined (PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include <gtk/gtk.h>

#include <papers-document.h>
#include <papers-view.h>

G_BEGIN_DECLS

#define PPS_TYPE_PIXBUF_CACHE            (pps_pixbuf_cache_get_type ())
#define PPS_PIXBUF_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_PIXBUF_CACHE, PpsPixbufCache))
#define PPS_IS_PIXBUF_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_PIXBUF_CACHE))



/* The coordinates in the rect here are at scale == 1.0, so that we can ignore
 * resizings.  There is one per page, maximum.
 */
typedef struct _PpsViewSelection PpsViewSelection;

struct _PpsViewSelection {
	int page;
	PpsRectangle rect;
	cairo_region_t *covered_region;
	PpsSelectionStyle style;
};

typedef struct _PpsPixbufCache       PpsPixbufCache;
typedef struct _PpsPixbufCacheClass  PpsPixbufCacheClass;

GType           pps_pixbuf_cache_get_type                (void) G_GNUC_CONST;
PpsPixbufCache  *pps_pixbuf_cache_new                     (GtkWidget       *view,
						         PpsDocumentModel *model,
						         gsize            max_size);
void            pps_pixbuf_cache_set_max_size            (PpsPixbufCache   *pixbuf_cache,
						         gsize            max_size);
void            pps_pixbuf_cache_set_page_range          (PpsPixbufCache   *pixbuf_cache,
						         gint             start_page,
						         gint             end_page,
						         GList           *selection_list);
GdkTexture     *pps_pixbuf_cache_get_texture             (PpsPixbufCache   *pixbuf_cache,
						         gint             page);
void            pps_pixbuf_cache_clear                   (PpsPixbufCache   *pixbuf_cache);
void            pps_pixbuf_cache_style_changed           (PpsPixbufCache   *pixbuf_cache);
void            pps_pixbuf_cache_reload_page 	        (PpsPixbufCache   *pixbuf_cache,
                    				         cairo_region_t  *region,
                    				         gint             page,
			                                 gint             rotation,
						         gdouble          scale);
/* Selection */
GdkTexture     *pps_pixbuf_cache_get_selection_texture   (PpsPixbufCache   *pixbuf_cache,
							 gint             page,
							 gfloat           scale);
cairo_region_t *pps_pixbuf_cache_get_selection_region    (PpsPixbufCache   *pixbuf_cache,
						         gint             page,
						         gfloat           scale);
void            pps_pixbuf_cache_set_selection_list      (PpsPixbufCache   *pixbuf_cache,
						         GList           *selection_list);
GList          *pps_pixbuf_cache_get_selection_list      (PpsPixbufCache   *pixbuf_cache);

G_END_DECLS
