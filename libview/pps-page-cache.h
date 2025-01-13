/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2009 Carlos Garcia Campos
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

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include <gdk/gdk.h>
#include <glib-object.h>
#include <papers-document.h>
#include <papers-view.h>

G_BEGIN_DECLS

#define PPS_TYPE_PAGE_CACHE (pps_page_cache_get_type ())
G_DECLARE_FINAL_TYPE (PpsPageCache, pps_page_cache, PPS, PAGE_CACHE, GObject)

PpsPageCache *pps_page_cache_new (PpsDocument *document);

void pps_page_cache_set_page_range (PpsPageCache *cache,
                                    gint start,
                                    gint end);
PpsJobPageDataFlags pps_page_cache_get_flags (PpsPageCache *cache);
void pps_page_cache_set_flags (PpsPageCache *cache,
                               PpsJobPageDataFlags flags);
void pps_page_cache_mark_dirty (PpsPageCache *cache,
                                gint page,
                                PpsJobPageDataFlags flags);
PpsMappingList *pps_page_cache_get_link_mapping (PpsPageCache *cache,
                                                 gint page);
PpsMappingList *pps_page_cache_get_image_mapping (PpsPageCache *cache,
                                                  gint page);
PpsMappingList *pps_page_cache_get_form_field_mapping (PpsPageCache *cache,
                                                       gint page);
PpsMappingList *pps_page_cache_get_annot_mapping (PpsPageCache *cache,
                                                  gint page);
PpsMappingList *pps_page_cache_get_media_mapping (PpsPageCache *cache,
                                                  gint page);
cairo_region_t *pps_page_cache_get_text_mapping (PpsPageCache *cache,
                                                 gint page);
const gchar *pps_page_cache_get_text (PpsPageCache *cache,
                                      gint page);
gboolean pps_page_cache_get_text_layout (PpsPageCache *cache,
                                         gint page,
                                         PpsRectangle **areas,
                                         guint *n_areas);
PangoAttrList *pps_page_cache_get_text_attrs (PpsPageCache *cache,
                                              gint page);
gboolean pps_page_cache_get_text_log_attrs (PpsPageCache *cache,
                                            gint page,
                                            PangoLogAttr **log_attrs,
                                            gulong *n_attrs);
void pps_page_cache_ensure_page (PpsPageCache *cache,
                                 gint page);
gboolean pps_page_cache_is_page_cached (PpsPageCache *cache,
                                        gint page);
G_END_DECLS
