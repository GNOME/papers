/*
 * Copyright (C) 2017, Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PPS_TYPE_ARCHIVE pps_archive_get_type ()
G_DECLARE_FINAL_TYPE (PpsArchive, pps_archive, PPS, ARCHIVE, GObject)

typedef enum {
	PPS_ARCHIVE_TYPE_NONE = 0,
	PPS_ARCHIVE_TYPE_RAR,
	PPS_ARCHIVE_TYPE_ZIP,
	PPS_ARCHIVE_TYPE_7Z,
	PPS_ARCHIVE_TYPE_TAR
} PpsArchiveType;

PpsArchive *pps_archive_new (void);
gboolean pps_archive_set_archive_type (PpsArchive *archive,
                                       PpsArchiveType archive_type);
PpsArchiveType pps_archive_get_archive_type (PpsArchive *archive);
gboolean pps_archive_open_filename (PpsArchive *archive,
                                    const char *path,
                                    GError **error);
gboolean pps_archive_read_next_header (PpsArchive *archive,
                                       GError **error);
gboolean pps_archive_at_entry (PpsArchive *archive);
const char *pps_archive_get_entry_pathname (PpsArchive *archive);
gint64 pps_archive_get_entry_size (PpsArchive *archive);
gboolean pps_archive_get_entry_is_encrypted (PpsArchive *archive);
gssize pps_archive_read_data (PpsArchive *archive,
                              void *buf,
                              gsize count,
                              GError **error);
void pps_archive_reset (PpsArchive *archive);

G_END_DECLS
