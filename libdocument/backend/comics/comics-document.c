// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2009-2010 Juanjo Marín <juanj.marin@juntadeandalucia.es>
 * Copyright (C) 2005, Teemu Tervo <teemu.tervo@gmx.net>
 * Copyright (C) 2016-2017, Bastien Nocera <hadess@hadess.net>
 */

#include <config.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include "comics-document.h"
#include "pps-archive.h"
#include "pps-document-misc.h"
#include "pps-file-helpers.h"

#define BLOCK_SIZE 10240

typedef struct _ComicsDocumentClass ComicsDocumentClass;

struct _ComicsDocumentClass {
	PpsDocumentClass parent_class;
};

struct _ComicsDocument {
	PpsDocument parent_instance;
	PpsArchive *archive;
	gchar *archive_path;
	gchar *archive_uri;
	GPtrArray *page_names;      /* elem: char * */
	GHashTable *page_positions; /* key: char *, value: uint + 1 */
	GRWLock rwlock;
};

G_DEFINE_TYPE (ComicsDocument, comics_document, PPS_TYPE_DOCUMENT)

#define FORMAT_UNKNOWN 0
#define FORMAT_SUPPORTED 1
#define FORMAT_UNSUPPORTED 2

/* Returns a GHashTable of:
 * <key>: file extensions
 * <value>: degree of support in gdk-pixbuf */
static GHashTable *
get_image_extensions (void)
{
	GHashTable *extensions;
	GSList *formats = gdk_pixbuf_get_formats ();
	GSList *l;
	guint i;
	const char *known_image_formats[] = {
		"png",
		"jpg",
		"jpeg",
		"webp"
	};

	extensions = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                    g_free, NULL);
	for (l = formats; l != NULL; l = l->next) {
		int i;
		gchar **ext = gdk_pixbuf_format_get_extensions (l->data);

		for (i = 0; ext[i] != NULL; i++) {
			g_hash_table_insert (extensions,
			                     g_strdup (ext[i]),
			                     GINT_TO_POINTER (FORMAT_SUPPORTED));
		}

		g_strfreev (ext);
	}

	g_slist_free (formats);

	/* Add known image formats that aren't supported by gdk-pixbuf */
	for (i = 0; i < G_N_ELEMENTS (known_image_formats); i++) {
		if (!g_hash_table_lookup (extensions, known_image_formats[i])) {
			g_hash_table_insert (extensions,
			                     g_strdup (known_image_formats[i]),
			                     GINT_TO_POINTER (FORMAT_UNSUPPORTED));
		}
	}

	return extensions;
}

static int
has_supported_extension (const char *name,
                         GHashTable *supported_extensions)
{
	gboolean ret = FALSE;
	gchar *suffix;

	suffix = g_strrstr (name, ".");
	if (!suffix)
		return ret;

	suffix = g_ascii_strdown (suffix + 1, -1);
	ret = GPOINTER_TO_INT (g_hash_table_lookup (supported_extensions, suffix));
	g_free (suffix);

	return ret;
}

#define APPLE_DOUBLE_PREFIX "._"
static gboolean
is_apple_double (const char *name)
{
	char *basename;
	gboolean ret = FALSE;

	basename = g_path_get_basename (name);
	if (basename == NULL) {
		g_debug ("Filename '%s' doesn't have a basename?", name);
		return ret;
	}

	ret = g_str_has_prefix (basename, APPLE_DOUBLE_PREFIX);
	g_free (basename);

	return ret;
}

static gboolean
archive_reopen_if_needed (ComicsDocument *comics_document,
                          const char *page_wanted,
                          GError **error)
{
	const char *current_page;
	guint current_page_idx, page_wanted_idx;

	if (pps_archive_at_entry (comics_document->archive)) {
		current_page = pps_archive_get_entry_pathname (comics_document->archive);
		if (current_page) {
			current_page_idx = GPOINTER_TO_UINT (g_hash_table_lookup (comics_document->page_positions, current_page));
			page_wanted_idx = GPOINTER_TO_UINT (g_hash_table_lookup (comics_document->page_positions, page_wanted));

			if (current_page_idx != 0 &&
			    page_wanted_idx != 0 &&
			    page_wanted_idx > current_page_idx)
				return TRUE;
		}

		pps_archive_reset (comics_document->archive);
	}

	return pps_archive_open_filename (comics_document->archive, comics_document->archive_path, error);
}

static GPtrArray *
comics_document_list (ComicsDocument *comics_document,
                      GError **error)
{
	GPtrArray *array = NULL;
	gboolean has_encrypted_files, has_unsupported_images, has_archive_errors;
	GHashTable *supported_extensions = NULL;

	has_encrypted_files = FALSE;
	has_unsupported_images = FALSE;
	has_archive_errors = FALSE;

	if (!pps_archive_open_filename (comics_document->archive, comics_document->archive_path, error)) {
		if (*error != NULL) {
			g_warning ("Fatal error handling archive (%s): %s", G_STRFUNC, (*error)->message);
			g_clear_error (error);
		}

		g_set_error_literal (error,
		                     PPS_DOCUMENT_ERROR,
		                     PPS_DOCUMENT_ERROR_INVALID,
		                     _ ("File is corrupted"));
		goto out;
	}

	supported_extensions = get_image_extensions ();

	array = g_ptr_array_sized_new (64);
	g_ptr_array_set_free_func (array, g_free);

	while (1) {
		const char *name;
		int supported;

		if (!pps_archive_read_next_header (comics_document->archive, error)) {
			if (*error != NULL) {
				g_debug ("Fatal error handling archive (%s): %s", G_STRFUNC, (*error)->message);
				g_clear_error (error);
				has_archive_errors = TRUE;
				goto out;
			}
			break;
		}

		name = pps_archive_get_entry_pathname (comics_document->archive);
		/* Ignore https://en.wikipedia.org/wiki/AppleSingle_and_AppleDouble_formats */
		if (is_apple_double (name)) {
			g_debug ("Not adding AppleDouble file '%s' to the list of files in the comics", name);
			continue;
		}

		supported = has_supported_extension (name, supported_extensions);
		if (supported == FORMAT_UNKNOWN) {
			g_debug ("Not adding unsupported file '%s' to the list of files in the comics", name);
			continue;
		} else if (supported == FORMAT_UNSUPPORTED) {
			g_debug ("Not adding unsupported image '%s' to the list of files in the comics", name);
			has_unsupported_images = TRUE;
			continue;
		}

		if (pps_archive_get_entry_is_encrypted (comics_document->archive)) {
			g_debug ("Not adding encrypted file '%s' to the list of files in the comics", name);
			has_encrypted_files = TRUE;
			continue;
		}

		g_debug ("Adding '%s' to the list of files in the comics", name);
		g_ptr_array_add (array, g_strdup (name));
	}

out:
	if (array->len == 0) {
		g_ptr_array_free (array, TRUE);
		array = NULL;

		if (has_encrypted_files) {
			g_set_error_literal (error,
			                     PPS_DOCUMENT_ERROR,
			                     PPS_DOCUMENT_ERROR_ENCRYPTED,
			                     _ ("Archive is encrypted"));
		} else if (has_unsupported_images) {
			g_set_error_literal (error,
			                     PPS_DOCUMENT_ERROR,
			                     PPS_DOCUMENT_ERROR_UNSUPPORTED_CONTENT,
			                     _ ("No supported images in archive"));
		} else if (has_archive_errors) {
			g_set_error_literal (error,
			                     PPS_DOCUMENT_ERROR,
			                     PPS_DOCUMENT_ERROR_INVALID,
			                     _ ("File is corrupted"));
		} else {
			g_set_error_literal (error,
			                     PPS_DOCUMENT_ERROR,
			                     PPS_DOCUMENT_ERROR_INVALID,
			                     _ ("No files in archive"));
		}
	}

	if (supported_extensions)
		g_hash_table_destroy (supported_extensions);
	pps_archive_reset (comics_document->archive);
	return array;
}

static GHashTable *
save_positions (GPtrArray *page_names)
{
	guint i;
	GHashTable *ht;

	ht = g_hash_table_new (g_str_hash, g_str_equal);
	for (i = 0; i < page_names->len; i++)
		g_hash_table_insert (ht, page_names->pdata[i], GUINT_TO_POINTER (i + 1));
	return ht;
}

/* This function chooses the archive decompression support
 * book based on its mime type. */
static gboolean
comics_check_decompress_support (gchar *mime_type,
                                 ComicsDocument *comics_document,
                                 GError **error)
{
	if (g_content_type_is_a (mime_type, "application/x-cbr") ||
	    g_content_type_is_a (mime_type, "application/x-rar")) {
		if (pps_archive_set_archive_type (comics_document->archive, PPS_ARCHIVE_TYPE_RAR))
			return TRUE;
	} else if (g_content_type_is_a (mime_type, "application/x-cbz") ||
	           g_content_type_is_a (mime_type, "application/zip")) {
		if (pps_archive_set_archive_type (comics_document->archive, PPS_ARCHIVE_TYPE_ZIP))
			return TRUE;
	} else if (g_content_type_is_a (mime_type, "application/x-cb7") ||
	           g_content_type_is_a (mime_type, "application/x-7z-compressed")) {
		if (pps_archive_set_archive_type (comics_document->archive, PPS_ARCHIVE_TYPE_7Z))
			return TRUE;
	} else if (g_content_type_is_a (mime_type, "application/x-cbt") ||
	           g_content_type_is_a (mime_type, "application/x-tar")) {
		if (pps_archive_set_archive_type (comics_document->archive, PPS_ARCHIVE_TYPE_TAR))
			return TRUE;
	} else {
		g_set_error (error,
		             PPS_DOCUMENT_ERROR,
		             PPS_DOCUMENT_ERROR_INVALID,
		             _ ("Not a comic book MIME type: %s"),
		             mime_type);
		return FALSE;
	}
	g_set_error_literal (error,
	                     PPS_DOCUMENT_ERROR,
	                     PPS_DOCUMENT_ERROR_INVALID,
	                     _ ("libarchive lacks support for this comic book’s "
	                        "compression, please contact your distributor"));
	return FALSE;
}

static int
sort_page_names (gconstpointer a,
                 gconstpointer b)
{
	gchar *temp1, *temp2;
	gint ret;

	temp1 = g_utf8_collate_key_for_filename (*(const char **) a, -1);
	temp2 = g_utf8_collate_key_for_filename (*(const char **) b, -1);

	ret = strcmp (temp1, temp2);

	g_free (temp1);
	g_free (temp2);

	return ret;
}

static gboolean
comics_document_load (PpsDocument *document,
                      const char *uri,
                      GError **error)
{
	ComicsDocument *comics_document = COMICS_DOCUMENT (document);
	g_autofree gchar *mime_type = NULL;
	g_autoptr (GFile) file = g_file_new_for_uri (uri);

	g_rw_lock_writer_lock (&comics_document->rwlock);

	comics_document->archive_path = g_file_get_path (file);
	if (!comics_document->archive_path) {
		g_set_error_literal (error,
		                     PPS_DOCUMENT_ERROR,
		                     PPS_DOCUMENT_ERROR_INVALID,
		                     _ ("Can not get local path for archive"));
		g_rw_lock_writer_unlock (&comics_document->rwlock);
		return FALSE;
	}

	comics_document->archive_uri = g_strdup (uri);

	mime_type = pps_file_get_mime_type (uri, FALSE, error);
	if (mime_type == NULL) {
		g_rw_lock_writer_unlock (&comics_document->rwlock);
		return FALSE;
	}

	if (!comics_check_decompress_support (mime_type, comics_document, error)) {
		g_rw_lock_writer_unlock (&comics_document->rwlock);
		return FALSE;
	}

	/* Get list of files in archive */
	comics_document->page_names = comics_document_list (comics_document, error);
	if (!comics_document->page_names) {
		g_rw_lock_writer_unlock (&comics_document->rwlock);
		return FALSE;
	}

	/* Keep an index */
	comics_document->page_positions = save_positions (comics_document->page_names);

	/* Now sort the pages */
	g_ptr_array_sort (comics_document->page_names, sort_page_names);

	g_rw_lock_writer_unlock (&comics_document->rwlock);

	return TRUE;
}

static gboolean
comics_document_save (PpsDocument *document,
                      const char *uri,
                      GError **error)
{
	ComicsDocument *comics_document = COMICS_DOCUMENT (document);
	gboolean transfer_success;

	g_rw_lock_reader_lock (&comics_document->rwlock);

	transfer_success = pps_xfer_uri_simple (comics_document->archive_uri, uri, error);

	g_rw_lock_reader_unlock (&comics_document->rwlock);

	return transfer_success;
}

static int
comics_document_get_n_pages (PpsDocument *document)
{
	ComicsDocument *comics_document = COMICS_DOCUMENT (document);
	int n_pages;

	g_rw_lock_reader_lock (&comics_document->rwlock);

	if (comics_document->page_names == NULL) {
		g_rw_lock_reader_unlock (&comics_document->rwlock);
		return 0;
	}

	n_pages = comics_document->page_names->len;

	g_rw_lock_reader_unlock (&comics_document->rwlock);

	return n_pages;
}

typedef struct
{
	gboolean got_info;
	int height;
	int width;
} PixbufInfo;

static void
get_page_size_prepared_cb (GdkPixbufLoader *loader,
                           int width,
                           int height,
                           PixbufInfo *info)
{
	info->got_info = TRUE;
	info->height = height;
	info->width = width;
}

static void
comics_document_get_page_size (PpsDocument *document,
                               PpsPage *page,
                               double *width,
                               double *height)
{
	GdkPixbufLoader *loader;
	ComicsDocument *comics_document = COMICS_DOCUMENT (document);
	const char *page_path;
	PixbufInfo info;
	GError *error = NULL;

	g_rw_lock_reader_lock (&comics_document->rwlock);

	page_path = g_ptr_array_index (comics_document->page_names, page->index);

	if (!archive_reopen_if_needed (comics_document, page_path, &error)) {
		g_warning ("Fatal error opening archive: %s", error->message);
		g_error_free (error);
		g_rw_lock_reader_unlock (&comics_document->rwlock);
		return;
	}

	loader = gdk_pixbuf_loader_new ();
	info.got_info = FALSE;
	g_signal_connect (loader, "size-prepared",
	                  G_CALLBACK (get_page_size_prepared_cb),
	                  &info);

	while (1) {
		const char *name;
		GError *error = NULL;

		if (!pps_archive_read_next_header (comics_document->archive, &error)) {
			if (error != NULL) {
				g_warning ("Fatal error handling archive (%s): %s", G_STRFUNC, error->message);
				g_error_free (error);
			}
			break;
		}

		name = pps_archive_get_entry_pathname (comics_document->archive);
		if (g_strcmp0 (name, page_path) == 0) {
			char buf[BLOCK_SIZE];
			gssize read;
			gint64 left;

			left = pps_archive_get_entry_size (comics_document->archive);
			read = pps_archive_read_data (comics_document->archive, buf,
			                              MIN (BLOCK_SIZE, left), &error);
			while (read > 0 && !info.got_info) {
				if (!gdk_pixbuf_loader_write (loader, (guchar *) buf, read, &error)) {
					read = -1;
					break;
				}
				left -= read;
				read = pps_archive_read_data (comics_document->archive, buf,
				                              MIN (BLOCK_SIZE, left), &error);
			}
			if (read < 0) {
				g_warning ("Fatal error reading '%s' in archive: %s", name, error->message);
				g_error_free (error);
			}
			break;
		}
	}

	gdk_pixbuf_loader_close (loader, NULL);
	g_object_unref (loader);

	if (info.got_info) {
		if (width)
			*width = info.width;
		if (height)
			*height = info.height;
	}

	g_rw_lock_reader_unlock (&comics_document->rwlock);
}

static void
render_pixbuf_size_prepared_cb (GdkPixbufLoader *loader,
                                gint width,
                                gint height,
                                PpsRenderContext *rc)
{
	int scaled_width, scaled_height;

	pps_render_context_compute_scaled_size (rc, width, height, &scaled_width, &scaled_height);
	gdk_pixbuf_loader_set_size (loader, scaled_width, scaled_height);
}

/* This function assumes that the caller has already acquired the necessary lock.
 * If called independently, ensure proper synchronization before invoking it.
 */
static GdkPixbuf *
comics_document_render_pixbuf (PpsDocument *document,
                               PpsRenderContext *rc)
{
	GdkPixbufLoader *loader;
	GdkPixbuf *tmp_pixbuf;
	GdkPixbuf *rotated_pixbuf = NULL;
	ComicsDocument *comics_document = COMICS_DOCUMENT (document);
	const char *page_path;
	GError *error = NULL;

	page_path = g_ptr_array_index (comics_document->page_names, rc->page->index);

	if (!archive_reopen_if_needed (comics_document, page_path, &error)) {
		g_warning ("Fatal error opening archive: %s", error->message);
		g_error_free (error);
		return NULL;
	}

	loader = gdk_pixbuf_loader_new ();
	g_signal_connect (loader, "size-prepared",
	                  G_CALLBACK (render_pixbuf_size_prepared_cb),
	                  rc);

	while (1) {
		const char *name;

		if (!pps_archive_read_next_header (comics_document->archive, &error)) {
			if (error != NULL) {
				g_warning ("Fatal error handling archive (%s): %s", G_STRFUNC, error->message);
				g_error_free (error);
			}
			break;
		}

		name = pps_archive_get_entry_pathname (comics_document->archive);
		if (g_strcmp0 (name, page_path) == 0) {
			size_t size = pps_archive_get_entry_size (comics_document->archive);
			char *buf;
			ssize_t read;

			buf = g_malloc (size);
			read = pps_archive_read_data (comics_document->archive, buf, size, &error);
			if (read <= 0) {
				if (read < 0) {
					g_warning ("Fatal error reading '%s' in archive: %s", name, error->message);
					g_error_free (error);
				} else {
					g_warning ("Read an empty file from the archive");
				}
			} else {
				gdk_pixbuf_loader_write (loader, (guchar *) buf, size, NULL);
			}
			g_free (buf);
			gdk_pixbuf_loader_close (loader, NULL);
			break;
		}
	}

	tmp_pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	if (tmp_pixbuf) {
		if ((rc->rotation % 360) == 0)
			rotated_pixbuf = g_object_ref (tmp_pixbuf);
		else
			rotated_pixbuf = gdk_pixbuf_rotate_simple (tmp_pixbuf,
			                                           360 - rc->rotation);
	}
	g_object_unref (loader);

	return rotated_pixbuf;
}

static cairo_surface_t *
comics_document_render (PpsDocument *document,
                        PpsRenderContext *rc)
{
	ComicsDocument *comics_document = COMICS_DOCUMENT (document);
	GdkPixbuf *pixbuf;
	cairo_surface_t *surface;

	g_rw_lock_reader_lock (&comics_document->rwlock);

	pixbuf = comics_document_render_pixbuf (document, rc);
	if (!pixbuf) {
		g_rw_lock_reader_unlock (&comics_document->rwlock);
		return NULL;
	}

	surface = pps_document_misc_surface_from_pixbuf (pixbuf);
	g_clear_object (&pixbuf);

	g_rw_lock_reader_unlock (&comics_document->rwlock);

	return surface;
}

static void
comics_document_finalize (GObject *object)
{
	ComicsDocument *comics_document = COMICS_DOCUMENT (object);

	if (comics_document->page_names)
		g_ptr_array_free (comics_document->page_names, TRUE);

	g_clear_pointer (&comics_document->page_positions, g_hash_table_destroy);
	g_clear_object (&comics_document->archive);
	g_free (comics_document->archive_path);
	g_free (comics_document->archive_uri);
	g_rw_lock_clear (&comics_document->rwlock);

	G_OBJECT_CLASS (comics_document_parent_class)->finalize (object);
}

static void
comics_document_class_init (ComicsDocumentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	PpsDocumentClass *pps_document_class = PPS_DOCUMENT_CLASS (klass);

	gobject_class->finalize = comics_document_finalize;

	pps_document_class->load = comics_document_load;
	pps_document_class->save = comics_document_save;
	pps_document_class->get_n_pages = comics_document_get_n_pages;
	pps_document_class->get_page_size = comics_document_get_page_size;
	pps_document_class->render = comics_document_render;
}

static void
comics_document_init (ComicsDocument *comics_document)
{
	comics_document->archive = pps_archive_new ();
	g_rw_lock_init (&comics_document->rwlock);
}

GType
pps_backend_query_type (void)
{
	return COMICS_TYPE_DOCUMENT;
}
