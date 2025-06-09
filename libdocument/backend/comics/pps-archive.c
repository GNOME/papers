// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2017, Bastien Nocera <hadess@hadess.net>
 */

#include "pps-archive.h"
#include "config.h"

#include <archive.h>
#include <archive_entry.h>
#include <gio/gio.h>

#define BUFFER_SIZE (64 * 1024)

struct _PpsArchive {
	GObject parent_instance;
	PpsArchiveType type;

	/* libarchive */
	struct archive *libar;
	struct archive_entry *libar_entry;
};

G_DEFINE_TYPE (PpsArchive, pps_archive, G_TYPE_OBJECT);

static void
pps_archive_finalize (GObject *object)
{
	PpsArchive *archive = PPS_ARCHIVE (object);

	switch (archive->type) {
	case PPS_ARCHIVE_TYPE_RAR:
	case PPS_ARCHIVE_TYPE_ZIP:
	case PPS_ARCHIVE_TYPE_7Z:
	case PPS_ARCHIVE_TYPE_TAR:
		g_clear_pointer (&archive->libar, archive_free);
		break;
	default:
		break;
	}

	G_OBJECT_CLASS (pps_archive_parent_class)->finalize (object);
}

static void
pps_archive_class_init (PpsArchiveClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	object_class->finalize = pps_archive_finalize;
}

PpsArchive *
pps_archive_new (void)
{
	return g_object_new (PPS_TYPE_ARCHIVE, NULL);
}

static void
libarchive_set_archive_type (PpsArchive *archive,
                             PpsArchiveType archive_type)
{
	archive->type = archive_type;
	archive->libar = archive_read_new ();

	if (archive_type == PPS_ARCHIVE_TYPE_ZIP)
		archive_read_support_format_zip (archive->libar);
	else if (archive_type == PPS_ARCHIVE_TYPE_7Z)
		archive_read_support_format_7zip (archive->libar);
	else if (archive_type == PPS_ARCHIVE_TYPE_TAR)
		archive_read_support_format_tar (archive->libar);
	else if (archive_type == PPS_ARCHIVE_TYPE_RAR) {
		archive_read_support_format_rar (archive->libar);
		archive_read_support_format_rar5 (archive->libar);
	} else
		g_assert_not_reached ();
}

PpsArchiveType
pps_archive_get_archive_type (PpsArchive *archive)
{
	g_return_val_if_fail (PPS_IS_ARCHIVE (archive), PPS_ARCHIVE_TYPE_NONE);

	return archive->type;
}

gboolean
pps_archive_set_archive_type (PpsArchive *archive,
                              PpsArchiveType archive_type)
{
	g_return_val_if_fail (PPS_IS_ARCHIVE (archive), FALSE);
	g_return_val_if_fail (archive->type == PPS_ARCHIVE_TYPE_NONE, FALSE);
	libarchive_set_archive_type (archive, archive_type);

	return TRUE;
}

gboolean
pps_archive_open_filename (PpsArchive *archive,
                           const char *path,
                           GError **error)
{
	int r;

	g_return_val_if_fail (PPS_IS_ARCHIVE (archive), FALSE);
	g_return_val_if_fail (archive->type != PPS_ARCHIVE_TYPE_NONE, FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	r = archive_read_open_filename (archive->libar, path, BUFFER_SIZE);
	if (r != ARCHIVE_OK) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
		             "Error opening archive: %s", archive_error_string (archive->libar));
		return FALSE;
	}
	return TRUE;
}

static gboolean
libarchive_read_next_header (PpsArchive *archive,
                             GError **error)
{
	while (1) {
		int r;

		r = archive_read_next_header (archive->libar, &archive->libar_entry);
		if (r != ARCHIVE_OK) {
			if (r != ARCHIVE_EOF)
				g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
				             "Error reading archive: %s", archive_error_string (archive->libar));
			return FALSE;
		}

		if (archive_entry_filetype (archive->libar_entry) != AE_IFREG) {
			g_debug ("Skipping '%s' as it's not a regular file",
			         archive_entry_pathname (archive->libar_entry));
			continue;
		}

		g_debug ("At header for file '%s'", archive_entry_pathname (archive->libar_entry));

		break;
	}

	return TRUE;
}

gboolean
pps_archive_read_next_header (PpsArchive *archive,
                              GError **error)
{
	g_return_val_if_fail (PPS_IS_ARCHIVE (archive), FALSE);
	g_return_val_if_fail (archive->type != PPS_ARCHIVE_TYPE_NONE, FALSE);

	return libarchive_read_next_header (archive, error);
}

gboolean
pps_archive_at_entry (PpsArchive *archive)
{
	g_return_val_if_fail (PPS_IS_ARCHIVE (archive), FALSE);
	g_return_val_if_fail (archive->type != PPS_ARCHIVE_TYPE_NONE, FALSE);

	return (archive->libar_entry != NULL);
}

const char *
pps_archive_get_entry_pathname (PpsArchive *archive)
{
	g_return_val_if_fail (PPS_IS_ARCHIVE (archive), NULL);
	g_return_val_if_fail (archive->type != PPS_ARCHIVE_TYPE_NONE, NULL);
	g_return_val_if_fail (archive->libar_entry != NULL, NULL);

	return archive_entry_pathname (archive->libar_entry);
}

gint64
pps_archive_get_entry_size (PpsArchive *archive)
{
	g_return_val_if_fail (PPS_IS_ARCHIVE (archive), -1);
	g_return_val_if_fail (archive->type != PPS_ARCHIVE_TYPE_NONE, -1);
	g_return_val_if_fail (archive->libar_entry != NULL, -1);

	return archive_entry_size (archive->libar_entry);
}

gboolean
pps_archive_get_entry_is_encrypted (PpsArchive *archive)
{
	g_return_val_if_fail (PPS_IS_ARCHIVE (archive), FALSE);
	g_return_val_if_fail (archive->type != PPS_ARCHIVE_TYPE_NONE, FALSE);
	g_return_val_if_fail (archive->libar_entry != NULL, -1);

	return archive_entry_is_encrypted (archive->libar_entry);
}

gssize
pps_archive_read_data (PpsArchive *archive,
                       void *buf,
                       gsize count,
                       GError **error)
{
	gssize r = -1;

	g_return_val_if_fail (PPS_IS_ARCHIVE (archive), -1);
	g_return_val_if_fail (archive->type != PPS_ARCHIVE_TYPE_NONE, -1);
	g_return_val_if_fail (archive->libar_entry != NULL, -1);

	r = archive_read_data (archive->libar, buf, count);
	if (r < 0) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
		             "Failed to decompress data: %s", archive_error_string (archive->libar));
	}

	return r;
}

void
pps_archive_reset (PpsArchive *archive)
{
	g_return_if_fail (PPS_IS_ARCHIVE (archive));
	g_return_if_fail (archive->type != PPS_ARCHIVE_TYPE_NONE);

	g_clear_pointer (&archive->libar, archive_free);
	libarchive_set_archive_type (archive, archive->type);
	archive->libar_entry = NULL;
}

static void
pps_archive_init (PpsArchive *archive)
{
}
