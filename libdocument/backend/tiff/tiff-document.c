// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005, Jonathan Blandford <jrb@gnome.org>
 */

/* FIXME: Should probably buffer calls to libtiff with TIFFSetWarningHandler
 */

#include "config.h"

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <stdint.h>
#include <stdio.h>

#include "pps-document-info.h"
#include "pps-document-misc.h"
#include "pps-file-exporter.h"
#include "pps-file-helpers.h"
#include "tiff-document.h"
#include "tiff2ps.h"
#include "tiffio.h"

struct _TiffDocumentClass {
	PpsDocumentClass parent_class;
};

struct _TiffDocument {
	PpsDocument parent_instance;

	TIFF *tiff;
	gint n_pages;
	TIFF2PSContext *ps_export_ctx;

	gchar *uri;
	GRWLock rwlock;
};

typedef struct _TiffDocumentClass TiffDocumentClass;

static void tiff_document_document_file_exporter_iface_init (PpsFileExporterInterface *iface);

G_DEFINE_TYPE_WITH_CODE (TiffDocument, tiff_document, PPS_TYPE_DOCUMENT, G_IMPLEMENT_INTERFACE (PPS_TYPE_FILE_EXPORTER, tiff_document_document_file_exporter_iface_init))

static TIFFErrorHandler orig_error_handler = NULL;
static TIFFErrorHandler orig_warning_handler = NULL;

static void
push_handlers (void)
{
	orig_error_handler = TIFFSetErrorHandler (NULL);
	orig_warning_handler = TIFFSetWarningHandler (NULL);
}

static void
pop_handlers (void)
{
	TIFFSetErrorHandler (orig_error_handler);
	TIFFSetWarningHandler (orig_warning_handler);
}

static gboolean
tiff_document_load (PpsDocument *document,
                    const char *uri,
                    GError **error)
{
	TiffDocument *tiff_document = TIFF_DOCUMENT (document);
	gchar *filename;
	TIFF *tiff;

	g_rw_lock_writer_lock (&tiff_document->rwlock);

	filename = g_filename_from_uri (uri, NULL, error);
	if (!filename) {
		g_rw_lock_writer_unlock (&tiff_document->rwlock);
		return FALSE;
	}

	push_handlers ();

	tiff = TIFFOpen (filename, "r");
	if (tiff) {
		guint32 w, h;

		/* FIXME: unused data? why bother here */
		TIFFGetField (tiff, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField (tiff, TIFFTAG_IMAGELENGTH, &h);
	}

	if (!tiff) {
		pop_handlers ();

		g_set_error_literal (error,
		                     PPS_DOCUMENT_ERROR,
		                     PPS_DOCUMENT_ERROR_INVALID,
		                     _ ("Invalid document"));

		g_free (filename);
		g_rw_lock_writer_unlock (&tiff_document->rwlock);
		return FALSE;
	}

	tiff_document->tiff = tiff;
	g_free (tiff_document->uri);
	g_free (filename);
	tiff_document->uri = g_strdup (uri);

	pop_handlers ();
	g_rw_lock_writer_unlock (&tiff_document->rwlock);
	return TRUE;
}

static gboolean
tiff_document_save (PpsDocument *document,
                    const char *uri,
                    GError **error)
{
	TiffDocument *tiff_document = TIFF_DOCUMENT (document);
	gboolean success;

	g_rw_lock_writer_lock (&tiff_document->rwlock);

	success = pps_xfer_uri_simple (tiff_document->uri, uri, error);

	g_rw_lock_writer_unlock (&tiff_document->rwlock);

	return success;
}

static int
tiff_document_get_n_pages (PpsDocument *document)
{
	TiffDocument *tiff_document = TIFF_DOCUMENT (document);
	int n_pages;

	g_rw_lock_writer_lock (&tiff_document->rwlock);

	g_return_val_if_fail (TIFF_IS_DOCUMENT (document), 0);
	g_return_val_if_fail (tiff_document->tiff != NULL, 0);

	if (tiff_document->n_pages == -1) {
		push_handlers ();
		tiff_document->n_pages = 0;

		do {
			tiff_document->n_pages++;
		} while (TIFFReadDirectory (tiff_document->tiff));
		pop_handlers ();
	}

	n_pages = tiff_document->n_pages;

	g_rw_lock_writer_unlock (&tiff_document->rwlock);

	return n_pages;
}

static void
tiff_document_get_resolution (TiffDocument *tiff_document,
                              gfloat *x_res,
                              gfloat *y_res)
{
	gfloat x = 0.0;
	gfloat y = 0.0;
	gushort unit;

	if (TIFFGetField (tiff_document->tiff, TIFFTAG_XRESOLUTION, &x) &&
	    TIFFGetField (tiff_document->tiff, TIFFTAG_YRESOLUTION, &y)) {
		if (TIFFGetFieldDefaulted (tiff_document->tiff, TIFFTAG_RESOLUTIONUNIT, &unit)) {
			if (unit == RESUNIT_CENTIMETER) {
				x *= 2.54;
				y *= 2.54;
			}
		}
	}

	/* Handle 0 values: some software set TIFF resolution as `0 , 0` see bug #646414 */
	*x_res = x > 0 ? x : 72.0;
	*y_res = y > 0 ? y : 72.0;
}

static void
tiff_document_get_page_size (PpsDocument *document,
                             PpsPage *page,
                             double *width,
                             double *height)
{
	guint32 w, h;
	gfloat x_res, y_res;
	TiffDocument *tiff_document = TIFF_DOCUMENT (document);

	g_rw_lock_reader_lock (&tiff_document->rwlock);

	g_return_if_fail (TIFF_IS_DOCUMENT (document));
	g_return_if_fail (tiff_document->tiff != NULL);

	push_handlers ();
	if (TIFFSetDirectory (tiff_document->tiff, page->index) != 1) {
		pop_handlers ();
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return;
	}

	TIFFGetField (tiff_document->tiff, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField (tiff_document->tiff, TIFFTAG_IMAGELENGTH, &h);
	tiff_document_get_resolution (tiff_document, &x_res, &y_res);
	h = h * (x_res / y_res);

	*width = w;
	*height = h;

	pop_handlers ();
	g_rw_lock_reader_unlock (&tiff_document->rwlock);
}

static cairo_surface_t *
tiff_document_render (PpsDocument *document,
                      PpsRenderContext *rc)
{
	TiffDocument *tiff_document = TIFF_DOCUMENT (document);
	int width, height;
	int scaled_width, scaled_height;
	float x_res, y_res;
	gint rowstride, bytes;
	guchar *pixels = NULL;
	guchar *p;
	int orientation;
	cairo_surface_t *surface;
	cairo_surface_t *rotated_surface;
	static const cairo_user_data_key_t key;

	g_rw_lock_reader_lock (&tiff_document->rwlock);

	g_return_val_if_fail (TIFF_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (tiff_document->tiff != NULL, NULL);

	push_handlers ();
	if (TIFFSetDirectory (tiff_document->tiff, rc->page->index) != 1) {
		pop_handlers ();
		g_warning ("Failed to select page %d", rc->page->index);
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	if (!TIFFGetField (tiff_document->tiff, TIFFTAG_IMAGEWIDTH, &width)) {
		pop_handlers ();
		g_warning ("Failed to read image width");
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	if (!TIFFGetField (tiff_document->tiff, TIFFTAG_IMAGELENGTH, &height)) {
		pop_handlers ();
		g_warning ("Failed to read image height");
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	if (!TIFFGetField (tiff_document->tiff, TIFFTAG_ORIENTATION, &orientation)) {
		orientation = ORIENTATION_TOPLEFT;
	}

	tiff_document_get_resolution (tiff_document, &x_res, &y_res);

	pop_handlers ();

	/* Sanity check the doc */
	if (width <= 0 || height <= 0) {
		g_warning ("Invalid width or height.");
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	rowstride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, width);
	if (rowstride / 4 != width) {
		g_warning ("Overflow while rendering document.");
		/* overflow, or cairo was changed in an unsupported way */
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	if (height >= INT_MAX / rowstride) {
		g_warning ("Overflow while rendering document.");
		/* overflow */
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}
	bytes = height * rowstride;

	pixels = g_try_malloc (bytes);
	if (!pixels) {
		g_warning ("Failed to allocate memory for rendering.");
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	if (!TIFFReadRGBAImageOriented (tiff_document->tiff,
	                                width, height,
	                                (uint32_t *) pixels,
	                                orientation, 0)) {
		g_warning ("Failed to read TIFF image.");
		g_free (pixels);
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	surface = cairo_image_surface_create_for_data (pixels,
	                                               CAIRO_FORMAT_RGB24,
	                                               width, height,
	                                               rowstride);
	cairo_surface_set_user_data (surface, &key,
	                             pixels, (cairo_destroy_func_t) g_free);
	pop_handlers ();

	g_rw_lock_reader_unlock (&tiff_document->rwlock);

	/* Convert the format returned by libtiff to
	 * what cairo expects
	 */
	p = pixels;
	while (p < pixels + bytes) {
		guint32 *pixel = (guint32 *) p;
		guint8 r = TIFFGetR (*pixel);
		guint8 g = TIFFGetG (*pixel);
		guint8 b = TIFFGetB (*pixel);
		guint8 a = TIFFGetA (*pixel);

		*pixel = (a << 24) | (r << 16) | (g << 8) | b;

		p += 4;
	}

	pps_render_context_compute_scaled_size (rc, width, height * (x_res / y_res),
	                                        &scaled_width, &scaled_height);
	rotated_surface = pps_document_misc_surface_rotate_and_scale (surface,
	                                                              scaled_width, scaled_height,
	                                                              rc->rotation);
	cairo_surface_destroy (surface);

	return rotated_surface;
}

static void
free_buffer (guchar *pixels, gpointer data)
{
	g_free (pixels);
}

static GdkPixbuf *
tiff_document_get_thumbnail (PpsDocument *document,
                             PpsRenderContext *rc)
{
	TiffDocument *tiff_document = TIFF_DOCUMENT (document);
	int width, height;
	int scaled_width, scaled_height;
	float x_res, y_res;
	gint rowstride, bytes;
	guchar *pixels = NULL;
	GdkPixbuf *pixbuf;
	GdkPixbuf *scaled_pixbuf;
	GdkPixbuf *rotated_pixbuf;

	g_rw_lock_reader_lock (&tiff_document->rwlock);

	push_handlers ();
	if (TIFFSetDirectory (tiff_document->tiff, rc->page->index) != 1) {
		pop_handlers ();
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	if (!TIFFGetField (tiff_document->tiff, TIFFTAG_IMAGEWIDTH, &width)) {
		pop_handlers ();
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	if (!TIFFGetField (tiff_document->tiff, TIFFTAG_IMAGELENGTH, &height)) {
		pop_handlers ();
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	tiff_document_get_resolution (tiff_document, &x_res, &y_res);

	pop_handlers ();

	/* Sanity check the doc */
	if (width <= 0 || height <= 0) {
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	if (width >= INT_MAX / 4) {
		/* overflow */
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}
	rowstride = width * 4;

	if (height >= INT_MAX / rowstride) {
		/* overflow */
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}
	bytes = height * rowstride;

	pixels = g_try_malloc (bytes);
	if (!pixels) {
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	if (!TIFFReadRGBAImageOriented (tiff_document->tiff,
	                                width, height,
	                                (uint32_t *) pixels,
	                                ORIENTATION_TOPLEFT, 0)) {
		g_free (pixels);
		g_rw_lock_reader_unlock (&tiff_document->rwlock);
		return NULL;
	}

	pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB, TRUE, 8,
	                                   width, height, rowstride,
	                                   free_buffer, NULL);
	pop_handlers ();

	g_rw_lock_reader_unlock (&tiff_document->rwlock);

	pps_render_context_compute_scaled_size (rc, width, height * (x_res / y_res),
	                                        &scaled_width, &scaled_height);
	scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
	                                         scaled_width, scaled_height,
	                                         GDK_INTERP_BILINEAR);
	g_object_unref (pixbuf);

	rotated_pixbuf = gdk_pixbuf_rotate_simple (scaled_pixbuf, 360 - rc->rotation);
	g_object_unref (scaled_pixbuf);

	return rotated_pixbuf;
}

static gchar *
tiff_document_get_page_label (PpsDocument *document,
                              PpsPage *page)
{
	TiffDocument *tiff_document = TIFF_DOCUMENT (document);
	static gchar *label;
	gchar *result;

	g_rw_lock_reader_lock (&tiff_document->rwlock);

	if (TIFFGetField (tiff_document->tiff, TIFFTAG_PAGENAME, &label) &&
	    g_utf8_validate (label, -1, NULL)) {
		result = g_strdup (label);
	} else {
		result = NULL;
	}

	g_rw_lock_reader_unlock (&tiff_document->rwlock);

	return result;
}

static PpsDocumentInfo *
tiff_document_get_info (PpsDocument *document)
{
	TiffDocument *tiff_document = TIFF_DOCUMENT (document);
	PpsDocumentInfo *info;
	const void *data;
	uint32_t size;

	info = pps_document_info_new ();

	g_rw_lock_reader_lock (&tiff_document->rwlock);

	if (TIFFGetField (tiff_document->tiff, TIFFTAG_XMLPACKET, &size, &data) == 1) {
		pps_document_info_set_from_xmp (info, (const char *) data, size);
	}

	g_rw_lock_reader_unlock (&tiff_document->rwlock);

	return info;
}

static void
tiff_document_finalize (GObject *object)
{
	TiffDocument *tiff_document = TIFF_DOCUMENT (object);

	if (tiff_document->tiff)
		TIFFClose (tiff_document->tiff);
	if (tiff_document->uri)
		g_free (tiff_document->uri);

	g_rw_lock_clear (&tiff_document->rwlock);

	G_OBJECT_CLASS (tiff_document_parent_class)->finalize (object);
}

static void
tiff_document_class_init (TiffDocumentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	PpsDocumentClass *pps_document_class = PPS_DOCUMENT_CLASS (klass);

	gobject_class->finalize = tiff_document_finalize;

	pps_document_class->load = tiff_document_load;
	pps_document_class->save = tiff_document_save;
	pps_document_class->get_n_pages = tiff_document_get_n_pages;
	pps_document_class->get_page_size = tiff_document_get_page_size;
	pps_document_class->render = tiff_document_render;
	pps_document_class->get_thumbnail = tiff_document_get_thumbnail;
	pps_document_class->get_page_label = tiff_document_get_page_label;
	pps_document_class->get_info = tiff_document_get_info;
}

/* postscript exporter implementation */
static void
tiff_document_file_exporter_begin (PpsFileExporter *exporter,
                                   PpsFileExporterContext *fc)
{
	TiffDocument *document = TIFF_DOCUMENT (exporter);

	document->ps_export_ctx = tiff2ps_context_new (fc->filename);
}

static void
tiff_document_file_exporter_do_page (PpsFileExporter *exporter, PpsRenderContext *rc)
{
	TiffDocument *document = TIFF_DOCUMENT (exporter);

	if (document->ps_export_ctx == NULL)
		return;
	if (TIFFSetDirectory (document->tiff, rc->page->index) != 1)
		return;
	tiff2ps_process_page (document->ps_export_ctx, document->tiff,
	                      0, 0, 0, 0, 0);
}

static void
tiff_document_file_exporter_end (PpsFileExporter *exporter)
{
	TiffDocument *document = TIFF_DOCUMENT (exporter);

	if (document->ps_export_ctx == NULL)
		return;
	tiff2ps_context_finalize (document->ps_export_ctx);
}

static PpsFileExporterCapabilities
tiff_document_file_exporter_get_capabilities (PpsFileExporter *exporter)
{
	return PPS_FILE_EXPORTER_CAN_PAGE_SET |
	       PPS_FILE_EXPORTER_CAN_COPIES |
	       PPS_FILE_EXPORTER_CAN_COLLATE |
	       PPS_FILE_EXPORTER_CAN_REVERSE |
	       PPS_FILE_EXPORTER_CAN_GENERATE_PS;
}

static void
tiff_document_document_file_exporter_iface_init (PpsFileExporterInterface *iface)
{
	iface->begin = tiff_document_file_exporter_begin;
	iface->do_page = tiff_document_file_exporter_do_page;
	iface->end = tiff_document_file_exporter_end;
	iface->get_capabilities = tiff_document_file_exporter_get_capabilities;
}

static void
tiff_document_init (TiffDocument *tiff_document)
{
	tiff_document->n_pages = -1;
	g_rw_lock_init (&tiff_document->rwlock);
}

GType
pps_backend_query_type (void)
{
	return TIFF_TYPE_DOCUMENT;
}
