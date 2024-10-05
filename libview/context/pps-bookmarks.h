/* pps-bookmarks.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2010 Carlos Garcia Campos  <carlosgc@gnome.org>
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

#include <libdocument/pps-macros.h>
#if !defined (__PPS_PAPERS_VIEW_H_INSIDE__) && !defined (PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-metadata.h"

G_BEGIN_DECLS

#define PPS_TYPE_BOOKMARK	   (pps_bookmark_get_type())
typedef struct _PpsBookmark {
        guint  page;
        gchar *title;
} PpsBookmark;

PPS_PUBLIC
GType pps_bookmark_get_type (void);

PPS_PUBLIC
PpsBookmark *pps_bookmark_new  (gint page, const gchar *title);

PPS_PUBLIC
PpsBookmark *pps_bookmark_copy (const PpsBookmark *bookmark);

PPS_PUBLIC
void         pps_bookmark_free (PpsBookmark *bookmark);

PPS_PUBLIC
guint	     pps_bookmark_get_page (const PpsBookmark *bookmark);

PPS_PUBLIC
const gchar *pps_bookmark_get_title (const PpsBookmark *bookmark);


PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsBookmarks, pps_bookmarks, PPS, BOOKMARKS, GObject)
#define PPS_TYPE_BOOKMARKS         (pps_bookmarks_get_type())

PPS_PUBLIC
PpsBookmarks *pps_bookmarks_new           (PpsMetadata *metadata);
PPS_PUBLIC
GList       *pps_bookmarks_get_bookmarks (PpsBookmarks *bookmarks);
PPS_PUBLIC
gboolean     pps_bookmarks_has_bookmarks (PpsBookmarks *bookmarks);
PPS_PUBLIC
void         pps_bookmarks_add           (PpsBookmarks *bookmarks,
                                         PpsBookmark  *bookmark);
PPS_PUBLIC
void         pps_bookmarks_delete        (PpsBookmarks *bookmarks,
                                         PpsBookmark  *bookmark);
PPS_PUBLIC
void         pps_bookmarks_update        (PpsBookmarks *bookmarks,
                                         PpsBookmark  *bookmark);

G_END_DECLS
