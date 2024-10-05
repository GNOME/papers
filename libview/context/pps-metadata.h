/* pps-metadata.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos  <carlosgc@gnome.org>
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
#include <gio/gio.h>

G_BEGIN_DECLS

PPS_PUBLIC
#define PPS_TYPE_METADATA         (pps_metadata_get_type())

G_DECLARE_FINAL_TYPE(PpsMetadata, pps_metadata, PPS, METADATA, GObject)

PPS_PUBLIC
PpsMetadata *pps_metadata_new                   (GFile       *file);
PPS_PUBLIC
gboolean    pps_metadata_is_empty              (PpsMetadata  *metadata);

PPS_PUBLIC
gboolean    pps_metadata_get_string            (PpsMetadata  *metadata,
					       const gchar *key,
					       const gchar     **value);
PPS_PUBLIC
gboolean    pps_metadata_set_string            (PpsMetadata  *metadata,
					       const gchar *key,
					       const gchar *value);
PPS_PUBLIC
gboolean    pps_metadata_get_int               (PpsMetadata  *metadata,
					       const gchar *key,
					       gint        *value);
PPS_PUBLIC
gboolean    pps_metadata_set_int               (PpsMetadata  *metadata,
					       const gchar *key,
					       gint         value);
PPS_PUBLIC
gboolean    pps_metadata_get_double            (PpsMetadata  *metadata,
					       const gchar *key,
					       gdouble     *value);
PPS_PUBLIC
gboolean    pps_metadata_set_double            (PpsMetadata  *metadata,
					       const gchar *key,
					       gdouble      value);
PPS_PUBLIC
gboolean    pps_metadata_get_boolean           (PpsMetadata  *metadata,
					       const gchar *key,
					       gboolean    *value);
PPS_PUBLIC
gboolean    pps_metadata_set_boolean           (PpsMetadata  *metadata,
					       const gchar *key,
					       gboolean     value);
PPS_PUBLIC
gboolean    pps_metadata_has_key               (PpsMetadata  *metadata,
                                               const gchar *key);

PPS_PUBLIC
gboolean    pps_metadata_is_file_supported (GFile       *file);

G_END_DECLS
