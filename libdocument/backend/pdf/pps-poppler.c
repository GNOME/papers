/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2018, Ppsangelos Rigas <erigas@rnd2.org>
 * Copyright (C) 2009, Juanjo Marín <juanj.marin@juntadeandalucia.es>
 * Copyright (C) 2004, Red Hat, Inc.
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

#include "config.h"
#include "pps-outlines.h"

#include <gtk/gtk.h>
#include <poppler.h>
#include <string.h>
#ifdef HAVE_CAIRO_PDF
#include <cairo-pdf.h>
#endif
#ifdef HAVE_CAIRO_PS
#include <cairo-ps.h>
#endif
#include <glib/gi18n-lib.h>

#include "pps-attachment.h"
#include "pps-document-annotations.h"
#include "pps-document-attachments.h"
#include "pps-document-find.h"
#include "pps-document-fonts.h"
#include "pps-document-forms.h"
#include "pps-document-images.h"
#include "pps-document-layers.h"
#include "pps-document-links.h"
#include "pps-document-media.h"
#include "pps-document-misc.h"
#include "pps-document-print.h"
#include "pps-document-security.h"
#include "pps-document-signatures.h"
#include "pps-document-text.h"
#include "pps-document-transition.h"
#include "pps-file-exporter.h"
#include "pps-file-helpers.h"
#include "pps-font-description.h"
#include "pps-form-field-private.h"
#include "pps-image.h"
#include "pps-media.h"
#include "pps-poppler.h"
#include "pps-selection.h"
#include "pps-signature.h"
#include "pps-transition-effect.h"

#if (defined(HAVE_CAIRO_PDF) || defined(HAVE_CAIRO_PS))
#define HAVE_CAIRO_PRINT
#endif

typedef struct
{
	PpsFileExporterFormat format;

	/* Pages per sheet */
	gint pages_per_sheet;
	gint pages_printed;
	gint pages_x;
	gint pages_y;
	gdouble paper_width;
	gdouble paper_height;

#ifdef HAVE_CAIRO_PRINT
	cairo_t *cr;
#else
	PopplerPSFile *ps_file;
#endif
} PdfPrintContext;

struct _PdfDocumentClass {
	PpsDocumentClass parent_class;
};

struct _PdfDocument {
	PpsDocument parent_instance;

	PopplerDocument *document;
	gchar *password;
	gboolean forms_modified;
	gboolean annots_modified;

	PopplerFontsIter *fonts_iter;
	gboolean missing_fonts;

	PdfPrintContext *print_ctx;

	GHashTable *annots;
};

static void pdf_document_security_iface_init (PpsDocumentSecurityInterface *iface);
static void pdf_document_document_links_iface_init (PpsDocumentLinksInterface *iface);
static void pdf_document_document_images_iface_init (PpsDocumentImagesInterface *iface);
static void pdf_document_document_forms_iface_init (PpsDocumentFormsInterface *iface);
static void pdf_document_document_fonts_iface_init (PpsDocumentFontsInterface *iface);
static void pdf_document_document_layers_iface_init (PpsDocumentLayersInterface *iface);
static void pdf_document_document_print_iface_init (PpsDocumentPrintInterface *iface);
static void pdf_document_document_annotations_iface_init (PpsDocumentAnnotationsInterface *iface);
static void pdf_document_document_attachments_iface_init (PpsDocumentAttachmentsInterface *iface);
static void pdf_document_document_media_iface_init (PpsDocumentMediaInterface *iface);
static void pdf_document_document_signatures_iface_init (PpsDocumentSignaturesInterface *iface);
static void pdf_document_find_iface_init (PpsDocumentFindInterface *iface);
static void pdf_document_file_exporter_iface_init (PpsFileExporterInterface *iface);
static void pdf_selection_iface_init (PpsSelectionInterface *iface);
static void pdf_document_page_transition_iface_init (PpsDocumentTransitionInterface *iface);
static void pdf_document_text_iface_init (PpsDocumentTextInterface *iface);
static int pdf_document_get_n_pages (PpsDocument *document);

static PpsLinkDest *pps_link_dest_from_dest (PdfDocument *self,
                                             PopplerDest *dest);
static PpsLink *pps_link_from_action (PdfDocument *self,
                                      PopplerAction *action);
static void pdf_print_context_free (PdfPrintContext *ctx);
static gboolean attachment_save_to_buffer (PopplerAttachment *attachment,
                                           gchar **buffer,
                                           gsize *buffer_size,
                                           GError **error);

static void
gdk_rgba_to_poppler_color (GdkRGBA *rgba, PopplerColor *poppler)
{
	poppler->red = CLAMP ((guint) (rgba->red * 65535), 0, 65535);
	poppler->green = CLAMP ((guint) (rgba->green * 65535), 0, 65535);
	poppler->blue = CLAMP ((guint) (rgba->blue * 65535), 0, 65535);
}

static void
poppler_color_to_gdk_rgba (PopplerColor *poppler_color, GdkRGBA *color)
{
	if (poppler_color) {
		color->red = CLAMP ((double) poppler_color->red / 65535.0, 0.0, 1.0);
		color->green = CLAMP ((double) poppler_color->green / 65535.0, 0.0, 1.0),
		color->blue = CLAMP ((double) poppler_color->blue / 65535.0, 0.0, 1.0),
		color->alpha = 1.0;
	} else { /* default color */
		*color = PPS_ANNOTATION_DEFAULT_COLOR;
	}
}

G_DEFINE_TYPE_WITH_CODE (PdfDocument,
                         pdf_document,
                         PPS_TYPE_DOCUMENT,
                         G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_SECURITY,
                                                pdf_document_security_iface_init)
                             G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_LINKS,
                                                    pdf_document_document_links_iface_init)
                                 G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_IMAGES,
                                                        pdf_document_document_images_iface_init)
                                     G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_FORMS,
                                                            pdf_document_document_forms_iface_init)
                                         G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_FONTS,
                                                                pdf_document_document_fonts_iface_init)
                                             G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_LAYERS,
                                                                    pdf_document_document_layers_iface_init)
                                                 G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_PRINT,
                                                                        pdf_document_document_print_iface_init)
                                                     G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_ANNOTATIONS,
                                                                            pdf_document_document_annotations_iface_init)
                                                         G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_ATTACHMENTS,
                                                                                pdf_document_document_attachments_iface_init)
                                                             G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_MEDIA,
                                                                                    pdf_document_document_media_iface_init)
                                                                 G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_SIGNATURES,
                                                                                        pdf_document_document_signatures_iface_init)
                                                                     G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_FIND,
                                                                                            pdf_document_find_iface_init)
                                                                         G_IMPLEMENT_INTERFACE (PPS_TYPE_FILE_EXPORTER,
                                                                                                pdf_document_file_exporter_iface_init)
                                                                             G_IMPLEMENT_INTERFACE (PPS_TYPE_SELECTION,
                                                                                                    pdf_selection_iface_init)
                                                                                 G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_TRANSITION,
                                                                                                        pdf_document_page_transition_iface_init)
                                                                                     G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_TEXT,
                                                                                                            pdf_document_text_iface_init));

static void
pdf_document_dispose (GObject *object)
{
	PdfDocument *self = PDF_DOCUMENT (object);

	g_clear_pointer (&self->print_ctx, pdf_print_context_free);
	g_clear_pointer (&self->annots, g_hash_table_destroy);
	g_clear_object (&self->document);
	g_clear_pointer (&self->fonts_iter, poppler_fonts_iter_free);

	G_OBJECT_CLASS (pdf_document_parent_class)->dispose (object);
}

static void
pdf_document_init (PdfDocument *self)
{
	self->password = NULL;
	self->annots = g_hash_table_new_full (g_direct_hash,
	                                      g_direct_equal,
	                                      (GDestroyNotify) NULL,
	                                      (GDestroyNotify) pps_mapping_list_unref);
}

static void
convert_error (GError *poppler_error,
               GError **error)
{
	if (poppler_error == NULL)
		return;

	if (poppler_error->domain == POPPLER_ERROR) {
		/* convert poppler errors into PpsDocument errors */
		gint code = PPS_DOCUMENT_ERROR_INVALID;
		if (poppler_error->code == POPPLER_ERROR_INVALID)
			code = PPS_DOCUMENT_ERROR_INVALID;
		else if (poppler_error->code == POPPLER_ERROR_ENCRYPTED)
			code = PPS_DOCUMENT_ERROR_ENCRYPTED;

		g_set_error_literal (error,
		                     PPS_DOCUMENT_ERROR,
		                     code,
		                     poppler_error->message);

		g_error_free (poppler_error);
	} else {
		g_propagate_error (error, poppler_error);
	}
}

static PopplerRectangle
pps_rect_to_poppler (PpsPage *page, const PpsRectangle *pps_rect)
{
	gdouble height;
	PopplerRectangle poppler_rect;

	poppler_page_get_size (POPPLER_PAGE (page->backend_page), NULL, &height);

	// Popplers' coordinate system starts at the bottom left, ours at
	// the top left
	poppler_rect.x1 = pps_rect->x1;
	poppler_rect.x2 = pps_rect->x2;
	poppler_rect.y1 = height - pps_rect->y2;
	poppler_rect.y2 = height - pps_rect->y1;

	return poppler_rect;
}

static PpsRectangle
poppler_rect_to_pps (PpsPage *page, const PopplerRectangle *poppler_rect)
{
	gdouble height;
	PpsRectangle pps_rect;

	poppler_page_get_size (POPPLER_PAGE (page->backend_page), NULL, &height);

	// Popplers' coordinate system starts at the bottom left, ours at
	// the top left
	pps_rect.x1 = poppler_rect->x1;
	pps_rect.x2 = poppler_rect->x2;
	pps_rect.y1 = height - poppler_rect->y2;
	pps_rect.y2 = height - poppler_rect->y1;

	return pps_rect;
}

/* PpsDocument */
static gboolean
pdf_document_save (PpsDocument *document,
                   const char *uri,
                   GError **error)
{
	PdfDocument *self = PDF_DOCUMENT (document);
	gboolean retval;
	GError *poppler_error = NULL;

	retval = poppler_document_save (self->document,
	                                uri, &poppler_error);
	if (retval) {
		self->forms_modified = FALSE;
		self->annots_modified = FALSE;
		pps_document_set_modified (PPS_DOCUMENT (document), FALSE);
	} else {
		convert_error (poppler_error, error);
	}

	return retval;
}

static gboolean
pdf_document_load (PpsDocument *document,
                   const char *uri,
                   GError **error)
{
	GError *poppler_error = NULL;
	PdfDocument *self = PDF_DOCUMENT (document);

	self->document =
	    poppler_document_new_from_file (uri, self->password, &poppler_error);

	if (self->document == NULL) {
		convert_error (poppler_error, error);
		return FALSE;
	}

	return TRUE;
}

static gboolean
pdf_document_load_stream (PpsDocument *document,
                          GInputStream *stream,
                          PpsDocumentLoadFlags flags,
                          GCancellable *cancellable,
                          GError **error)
{
	GError *err = NULL;
	PdfDocument *self = PDF_DOCUMENT (document);

	self->document =
	    poppler_document_new_from_stream (stream, -1,
	                                      self->password,
	                                      cancellable,
	                                      &err);

	if (self->document == NULL) {
		convert_error (err, error);
		return FALSE;
	}

	return TRUE;
}

static gboolean
pdf_document_load_gfile (PpsDocument *document,
                         GFile *file,
                         PpsDocumentLoadFlags flags,
                         GCancellable *cancellable,
                         GError **error)
{
	GError *err = NULL;
	PdfDocument *self = PDF_DOCUMENT (document);

	self->document =
	    poppler_document_new_from_gfile (file,
	                                     self->password,
	                                     cancellable,
	                                     &err);

	if (self->document == NULL) {
		convert_error (err, error);
		return FALSE;
	}

	return TRUE;
}

static gboolean
pdf_document_load_fd (PpsDocument *document,
                      int fd,
                      PpsDocumentLoadFlags flags,
                      GError **error)
{
	GError *err = NULL;
	PdfDocument *self = PDF_DOCUMENT (document);

	/* Note: this consumes @fd */
	self->document =
	    poppler_document_new_from_fd (fd,
	                                  self->password,
	                                  &err);

	if (self->document == NULL) {
		convert_error (err, error);
		return FALSE;
	}

	return TRUE;
}

static int
pdf_document_get_n_pages (PpsDocument *document)
{
	return poppler_document_get_n_pages (PDF_DOCUMENT (document)->document);
}

static PpsPage *
pdf_document_get_page (PpsDocument *document,
                       gint index)
{
	PdfDocument *self = PDF_DOCUMENT (document);
	PopplerPage *poppler_page;
	PpsPage *page;

	poppler_page = poppler_document_get_page (self->document, index);
	page = pps_page_new (index);
	page->backend_page = (PpsBackendPage) g_object_ref (poppler_page);
	page->backend_destroy_func = (PpsBackendPageDestroyFunc) g_object_unref;
	g_object_unref (poppler_page);

	return page;
}

static void
pdf_document_get_page_size (PpsDocument *document,
                            PpsPage *page,
                            double *width,
                            double *height)
{
	g_return_if_fail (POPPLER_IS_PAGE (page->backend_page));

	poppler_page_get_size (POPPLER_PAGE (page->backend_page), width, height);
}

static char *
pdf_document_get_page_label (PpsDocument *document,
                             PpsPage *page)
{
	char *label = NULL;

	g_return_val_if_fail (POPPLER_IS_PAGE (page->backend_page), NULL);

	g_object_get (G_OBJECT (page->backend_page),
	              "label", &label,
	              NULL);
	return label;
}

static cairo_surface_t *
pdf_page_render (PopplerPage *page,
                 gint width,
                 gint height,
                 PpsRenderContext *rc)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	double page_width, page_height;
	double xscale, yscale;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
	                                      width, height);
	cr = cairo_create (surface);

	switch (rc->rotation) {
	case 90:
		cairo_translate (cr, width, 0);
		break;
	case 180:
		cairo_translate (cr, width, height);
		break;
	case 270:
		cairo_translate (cr, 0, height);
		break;
	default:
		cairo_translate (cr, 0, 0);
	}

	poppler_page_get_size (page,
	                       &page_width, &page_height);

	pps_render_context_compute_scales (rc, page_width, page_height, &xscale, &yscale);
	cairo_scale (cr, xscale, yscale);
	cairo_rotate (cr, rc->rotation * G_PI / 180.0);
#ifdef HAVE_FREE_TEXT
	poppler_page_render_full (page, cr, FALSE, (PopplerRenderAnnotsFlags) rc->annot_flags);
#else
	poppler_page_render (page, cr);
#endif

	cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
	cairo_set_source_rgb (cr, 1., 1., 1.);
	cairo_paint (cr);

	cairo_destroy (cr);

	return surface;
}

static cairo_surface_t *
pdf_document_render (PpsDocument *document,
                     PpsRenderContext *rc)
{
	PopplerPage *poppler_page;
	double width_points, height_points;
	gint width, height;

	poppler_page = POPPLER_PAGE (rc->page->backend_page);

	poppler_page_get_size (poppler_page,
	                       &width_points, &height_points);

	pps_render_context_compute_transformed_size (rc, width_points, height_points,
	                                             &width, &height);
	return pdf_page_render (poppler_page,
	                        width, height, rc);
}

static GdkPixbuf *
make_thumbnail_for_page (PopplerPage *poppler_page,
                         PpsRenderContext *rc,
                         gint width,
                         gint height)
{
	GdkPixbuf *pixbuf;
	cairo_surface_t *surface;

	surface = pdf_page_render (poppler_page, width, height, rc);

	pixbuf = pps_document_misc_pixbuf_from_surface (surface);
	cairo_surface_destroy (surface);

	return pixbuf;
}

static GdkPixbuf *
pdf_document_get_thumbnail (PpsDocument *document,
                            PpsRenderContext *rc)
{
	PopplerPage *poppler_page;
	cairo_surface_t *surface;
	GdkPixbuf *pixbuf = NULL;
	double page_width, page_height;
	gint width, height;

	poppler_page = POPPLER_PAGE (rc->page->backend_page);

	poppler_page_get_size (poppler_page,
	                       &page_width, &page_height);

	pps_render_context_compute_transformed_size (rc, page_width, page_height,
	                                             &width, &height);

	surface = poppler_page_get_thumbnail (poppler_page);
	if (surface) {
		pixbuf = pps_document_misc_pixbuf_from_surface (surface);
		cairo_surface_destroy (surface);
	}

	if (pixbuf != NULL) {
		int thumb_width = (rc->rotation == 90 || rc->rotation == 270) ? gdk_pixbuf_get_height (pixbuf) : gdk_pixbuf_get_width (pixbuf);

		if (thumb_width == width) {
			GdkPixbuf *rotated_pixbuf;

			rotated_pixbuf = gdk_pixbuf_rotate_simple (pixbuf,
			                                           (GdkPixbufRotation) (360 - rc->rotation));
			g_object_unref (pixbuf);
			pixbuf = rotated_pixbuf;
		} else {
			/* The provided thumbnail has a different size */
			g_object_unref (pixbuf);
			pixbuf = make_thumbnail_for_page (poppler_page, rc, width, height);
		}
	} else {
		/* There is no provided thumbnail. We need to make one. */
		pixbuf = make_thumbnail_for_page (poppler_page, rc, width, height);
	}

	return pixbuf;
}

static cairo_surface_t *
pdf_document_get_thumbnail_surface (PpsDocument *document,
                                    PpsRenderContext *rc)
{

	PopplerPage *poppler_page;
	cairo_surface_t *surface;
	double page_width, page_height;
	gint width, height;

	poppler_page = POPPLER_PAGE (rc->page->backend_page);

	poppler_page_get_size (poppler_page,
	                       &page_width, &page_height);

	pps_render_context_compute_transformed_size (rc, page_width, page_height,
	                                             &width, &height);

	surface = poppler_page_get_thumbnail (poppler_page);
	if (surface) {
		int surface_width = (rc->rotation == 90 || rc->rotation == 270) ? cairo_image_surface_get_height (surface) : cairo_image_surface_get_width (surface);

		if (surface_width == width) {
			cairo_surface_t *rotated_surface;

			rotated_surface = pps_document_misc_surface_rotate_and_scale (surface, width, height, rc->rotation);
			cairo_surface_destroy (surface);
			return rotated_surface;
		} else {
			/* The provided thumbnail has a different size */
			cairo_surface_destroy (surface);
		}
	}

	surface = pdf_page_render (poppler_page, width, height, rc);

	return surface;
}

static PpsDocumentInfo *
pdf_document_get_info (PpsDocument *document)
{
	PpsDocumentInfo *info;
	PopplerPageLayout layout;
	PopplerPageMode mode;
	PopplerViewerPreferences view_prefs;
	PopplerPermissions permissions;
	char *metadata;
	gboolean linearized;
	GDateTime *created_datetime = NULL;
	GDateTime *modified_datetime = NULL;

	info = pps_document_info_new ();

	info->fields_mask |= PPS_DOCUMENT_INFO_LAYOUT |
	                     PPS_DOCUMENT_INFO_START_MODE |
	                     PPS_DOCUMENT_INFO_PERMISSIONS |
	                     PPS_DOCUMENT_INFO_UI_HINTS |
	                     PPS_DOCUMENT_INFO_LINEARIZED |
	                     PPS_DOCUMENT_INFO_N_PAGES |
	                     PPS_DOCUMENT_INFO_SECURITY |
	                     PPS_DOCUMENT_INFO_PAPER_SIZE;

	g_object_get (PDF_DOCUMENT (document)->document,
	              "title", &(info->title),
	              "format", &(info->format),
	              "author", &(info->author),
	              "subject", &(info->subject),
	              "keywords", &(info->keywords),
	              "page-mode", &mode,
	              "page-layout", &layout,
	              "viewer-preferences", &view_prefs,
	              "permissions", &permissions,
	              "creator", &(info->creator),
	              "producer", &(info->producer),
	              "creation-datetime", &created_datetime,
	              "mod-datetime", &modified_datetime,
	              "linearized", &linearized,
	              "metadata", &metadata,
	              NULL);

	if (info->title)
		info->fields_mask |= PPS_DOCUMENT_INFO_TITLE;
	if (info->format)
		info->fields_mask |= PPS_DOCUMENT_INFO_FORMAT;
	if (info->author)
		info->fields_mask |= PPS_DOCUMENT_INFO_AUTHOR;
	if (info->subject)
		info->fields_mask |= PPS_DOCUMENT_INFO_SUBJECT;
	if (info->keywords)
		info->fields_mask |= PPS_DOCUMENT_INFO_KEYWORDS;
	if (info->creator)
		info->fields_mask |= PPS_DOCUMENT_INFO_CREATOR;
	if (info->producer)
		info->fields_mask |= PPS_DOCUMENT_INFO_PRODUCER;

	pps_document_info_take_created_datetime (info, created_datetime);
	pps_document_info_take_modified_datetime (info, modified_datetime);

	if (metadata != NULL) {
		pps_document_info_set_from_xmp (info, metadata, -1);
		g_free (metadata);
	}

	info->n_pages = poppler_document_get_n_pages (PDF_DOCUMENT (document)->document);

	if (info->n_pages > 0) {
		PopplerPage *poppler_page;

		poppler_page = poppler_document_get_page (PDF_DOCUMENT (document)->document, 0);
		poppler_page_get_size (poppler_page, &(info->paper_width), &(info->paper_height));
		g_object_unref (poppler_page);

		// Convert to mm.
		info->paper_width = info->paper_width / 72.0f * 25.4f;
		info->paper_height = info->paper_height / 72.0f * 25.4f;
	}

	switch (layout) {
	case POPPLER_PAGE_LAYOUT_SINGLE_PAGE:
		info->layout = PPS_DOCUMENT_LAYOUT_SINGLE_PAGE;
		break;
	case POPPLER_PAGE_LAYOUT_ONE_COLUMN:
		info->layout = PPS_DOCUMENT_LAYOUT_ONE_COLUMN;
		break;
	case POPPLER_PAGE_LAYOUT_TWO_COLUMN_LEFT:
		info->layout = PPS_DOCUMENT_LAYOUT_TWO_COLUMN_LEFT;
		break;
	case POPPLER_PAGE_LAYOUT_TWO_COLUMN_RIGHT:
		info->layout = PPS_DOCUMENT_LAYOUT_TWO_COLUMN_RIGHT;
		break;
	case POPPLER_PAGE_LAYOUT_TWO_PAGE_LEFT:
		info->layout = PPS_DOCUMENT_LAYOUT_TWO_PAGE_LEFT;
		break;
	case POPPLER_PAGE_LAYOUT_TWO_PAGE_RIGHT:
		info->layout = PPS_DOCUMENT_LAYOUT_TWO_PAGE_RIGHT;
		break;
	default:
		break;
	}

	switch (mode) {
	case POPPLER_PAGE_MODE_NONE:
		info->mode = PPS_DOCUMENT_MODE_NONE;
		break;
	case POPPLER_PAGE_MODE_USE_THUMBS:
		info->mode = PPS_DOCUMENT_MODE_USE_THUMBS;
		break;
	case POPPLER_PAGE_MODE_USE_OC:
		info->mode = PPS_DOCUMENT_MODE_USE_OC;
		break;
	case POPPLER_PAGE_MODE_FULL_SCREEN:
		info->mode = PPS_DOCUMENT_MODE_FULL_SCREEN;
		break;
	case POPPLER_PAGE_MODE_USE_ATTACHMENTS:
		info->mode = PPS_DOCUMENT_MODE_USE_ATTACHMENTS;
	default:
		break;
	}

	info->ui_hints = 0;
	if (view_prefs & POPPLER_VIEWER_PREFERENCES_HIDE_TOOLBAR) {
		info->ui_hints |= PPS_DOCUMENT_UI_HINT_HIDE_TOOLBAR;
	}
	if (view_prefs & POPPLER_VIEWER_PREFERENCES_HIDE_MENUBAR) {
		info->ui_hints |= PPS_DOCUMENT_UI_HINT_HIDE_MENUBAR;
	}
	if (view_prefs & POPPLER_VIEWER_PREFERENCES_HIDE_WINDOWUI) {
		info->ui_hints |= PPS_DOCUMENT_UI_HINT_HIDE_WINDOWUI;
	}
	if (view_prefs & POPPLER_VIEWER_PREFERENCES_FIT_WINDOW) {
		info->ui_hints |= PPS_DOCUMENT_UI_HINT_FIT_WINDOW;
	}
	if (view_prefs & POPPLER_VIEWER_PREFERENCES_CENTER_WINDOW) {
		info->ui_hints |= PPS_DOCUMENT_UI_HINT_CENTER_WINDOW;
	}
	if (view_prefs & POPPLER_VIEWER_PREFERENCES_DISPLAY_DOC_TITLE) {
		info->ui_hints |= PPS_DOCUMENT_UI_HINT_DISPLAY_DOC_TITLE;
	}
	if (view_prefs & POPPLER_VIEWER_PREFERENCES_DIRECTION_RTL) {
		info->ui_hints |= PPS_DOCUMENT_UI_HINT_DIRECTION_RTL;
	}

	info->permissions = 0;
	if (permissions & POPPLER_PERMISSIONS_OK_TO_PRINT) {
		info->permissions |= PPS_DOCUMENT_PERMISSIONS_OK_TO_PRINT;
	}
	if (permissions & POPPLER_PERMISSIONS_OK_TO_MODIFY) {
		info->permissions |= PPS_DOCUMENT_PERMISSIONS_OK_TO_MODIFY;
	}
	if (permissions & POPPLER_PERMISSIONS_OK_TO_COPY) {
		info->permissions |= PPS_DOCUMENT_PERMISSIONS_OK_TO_COPY;
	}
	if (permissions & POPPLER_PERMISSIONS_OK_TO_ADD_NOTES) {
		info->permissions |= PPS_DOCUMENT_PERMISSIONS_OK_TO_ADD_NOTES;
	}

	if (pps_document_security_has_document_security (PPS_DOCUMENT_SECURITY (document))) {
		/* translators: this is the document security state */
		info->security = g_strdup (_ ("Yes"));
	} else {
		/* translators: this is the document security state */
		info->security = g_strdup (_ ("No"));
	}

	info->linearized = linearized ? g_strdup (_ ("Yes")) : g_strdup (_ ("No"));

	info->contains_js = poppler_document_has_javascript (PDF_DOCUMENT (document)->document) ? PPS_DOCUMENT_CONTAINS_JS_YES : PPS_DOCUMENT_CONTAINS_JS_NO;
	info->fields_mask |= PPS_DOCUMENT_INFO_CONTAINS_JS;

	return info;
}

static gboolean
pdf_document_get_backend_info (PpsDocument *document, PpsDocumentBackendInfo *info)
{
	PopplerBackend backend;

	backend = poppler_get_backend ();
	switch (backend) {
	case POPPLER_BACKEND_CAIRO:
		info->name = "poppler/cairo";
		break;
	case POPPLER_BACKEND_SPLASH:
		info->name = "poppler/splash";
		break;
	default:
		info->name = "poppler/unknown";
		break;
	}

	info->version = poppler_get_version ();

	return TRUE;
}

static void
pdf_document_class_init (PdfDocumentClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
	PpsDocumentClass *pps_document_class = PPS_DOCUMENT_CLASS (klass);

	g_object_class->dispose = pdf_document_dispose;

	pps_document_class->save = pdf_document_save;
	pps_document_class->load = pdf_document_load;
	pps_document_class->load_stream = pdf_document_load_stream;
	pps_document_class->load_gfile = pdf_document_load_gfile;
	pps_document_class->get_n_pages = pdf_document_get_n_pages;
	pps_document_class->get_page = pdf_document_get_page;
	pps_document_class->get_page_size = pdf_document_get_page_size;
	pps_document_class->get_page_label = pdf_document_get_page_label;
	pps_document_class->render = pdf_document_render;
	pps_document_class->get_thumbnail = pdf_document_get_thumbnail;
	pps_document_class->get_thumbnail_surface = pdf_document_get_thumbnail_surface;
	pps_document_class->get_info = pdf_document_get_info;
	pps_document_class->get_backend_info = pdf_document_get_backend_info;
	pps_document_class->load_fd = pdf_document_load_fd;
}

/* PpsDocumentSecurity */
static gboolean
pdf_document_has_document_security (PpsDocumentSecurity *document_security)
{
	/* FIXME: do we really need to have this? */
	return FALSE;
}

static void
pdf_document_set_password (PpsDocumentSecurity *document_security,
                           const char *password)
{
	PdfDocument *document = PDF_DOCUMENT (document_security);

	if (document->password)
		g_free (document->password);

	document->password = g_strdup (password);
}

static void
pdf_document_security_iface_init (PpsDocumentSecurityInterface *iface)
{
	iface->has_document_security = pdf_document_has_document_security;
	iface->set_password = pdf_document_set_password;
}

static void
pdf_document_fonts_scan (PpsDocumentFonts *document_fonts)
{
	PdfDocument *self = PDF_DOCUMENT (document_fonts);
	PopplerFontInfo *font_info;
	PopplerFontsIter *fonts_iter;
	int n_pages;

	g_return_if_fail (PDF_IS_DOCUMENT (document_fonts));

	font_info = poppler_font_info_new (self->document);
	n_pages = pps_document_get_n_pages (PPS_DOCUMENT (document_fonts));
	poppler_font_info_scan (font_info, n_pages, &fonts_iter);

	g_clear_pointer (&self->fonts_iter, poppler_fonts_iter_free);
	self->fonts_iter = fonts_iter;

	poppler_font_info_free (font_info);
}

static const char *
font_type_to_string (PopplerFontType type)
{
	switch (type) {
	case POPPLER_FONT_TYPE_TYPE1:
		return _ ("Type 1");
	case POPPLER_FONT_TYPE_TYPE1C:
		return _ ("Type 1C");
	case POPPLER_FONT_TYPE_TYPE3:
		return _ ("Type 3");
	case POPPLER_FONT_TYPE_TRUETYPE:
		return _ ("TrueType");
	case POPPLER_FONT_TYPE_CID_TYPE0:
		return _ ("Type 1 (CID)");
	case POPPLER_FONT_TYPE_CID_TYPE0C:
		return _ ("Type 1C (CID)");
	case POPPLER_FONT_TYPE_CID_TYPE2:
		return _ ("TrueType (CID)");
	default:
		return _ ("Unknown font type");
	}
}

static gboolean
is_standard_font (const gchar *name, PopplerFontType type)
{
	/* list borrowed from Poppler: poppler/GfxFont.cc */
	static const char *base_14_subst_fonts[14] = {
		"Courier",
		"Courier-Oblique",
		"Courier-Bold",
		"Courier-BoldOblique",
		"Helvetica",
		"Helvetica-Oblique",
		"Helvetica-Bold",
		"Helvetica-BoldOblique",
		"Times-Roman",
		"Times-Italic",
		"Times-Bold",
		"Times-BoldItalic",
		"Symbol",
		"ZapfDingbats"
	};
	unsigned int i;

	/* The Standard 14 fonts are all Type 1 fonts. A non embedded TrueType
	 * font with the same name is not a Standard 14 font. */
	if (type != POPPLER_FONT_TYPE_TYPE1)
		return FALSE;

	for (i = 0; i < G_N_ELEMENTS (base_14_subst_fonts); i++) {
		if (g_str_equal (name, base_14_subst_fonts[i]))
			return TRUE;
	}
	return FALSE;
}

static const gchar *
pdf_document_fonts_get_fonts_summary (PpsDocumentFonts *document_fonts)
{
	PdfDocument *self = PDF_DOCUMENT (document_fonts);

	if (self->missing_fonts)
		return _ ("This document contains non-embedded fonts that are not from the "
		          "PDF Standard 14 fonts. If the substitute fonts selected by fontconfig "
		          "are not the same as the fonts used to create the PDF, the rendering may "
		          "not be correct.");
	else
		return _ ("All fonts are either standard or embedded.");
}

static GListModel *
pdf_document_fonts_get_model (PpsDocumentFonts *document_fonts)
{
	PdfDocument *self = PDF_DOCUMENT (document_fonts);
	PopplerFontsIter *iter = self->fonts_iter;
	GListStore *model = NULL;

	g_return_val_if_fail (PDF_IS_DOCUMENT (document_fonts), NULL);

	if (!iter)
		return NULL;

	model = g_list_store_new (PPS_TYPE_FONT_DESCRIPTION);

	do {
		const char *name;
		PopplerFontType type;
		const char *type_str;
		const char *embedded;
		const char *standard_str = "";
		const gchar *substitute;
		const gchar *filename;
		const gchar *encoding;
		char *details;

		name = poppler_fonts_iter_get_name (iter);

		if (name == NULL) {
			name = _ ("No name");
		}

		encoding = poppler_fonts_iter_get_encoding (iter);
		if (!encoding) {
			/* translators: When a font type does not have
			   encoding information or it is unknown.  Example:
			   Encoding: None
			*/
			encoding = _ ("None");
		}

		type = poppler_fonts_iter_get_font_type (iter);
		type_str = font_type_to_string (type);

		if (poppler_fonts_iter_is_embedded (iter)) {
			if (poppler_fonts_iter_is_subset (iter))
				embedded = _ ("Embedded subset");
			else
				embedded = _ ("Embedded");
		} else {
			embedded = _ ("Not embedded");
			if (is_standard_font (name, type)) {
				/* Translators: string starting with a space
				 * because it is directly appended to the font
				 * type. Example:
				 * "Type 1 (One of the Standard 14 Fonts)"
				 */
				standard_str = _ (" (One of the Standard 14 Fonts)");
			} else {
				/* Translators: string starting with a space
				 * because it is directly appended to the font
				 * type. Example:
				 * "TrueType (Not one of the Standard 14 Fonts)"
				 */
				standard_str = _ (" (Not one of the Standard 14 Fonts)");
				self->missing_fonts = TRUE;
			}
		}

		substitute = poppler_fonts_iter_get_substitute_name (iter);
		filename = poppler_fonts_iter_get_file_name (iter);

		if (substitute && filename)
			/* Translators: string is a concatenation of previous
			 * translated strings to indicate the fonts properties
			 * in a PDF document.
			 *
			 * Example:
			 * Type 1 (One of the standard 14 Fonts)
			 * Not embedded
			 * Substituting with TeXGyreTermes-Regular
			 * (/usr/share/textmf/.../texgyretermes-regular.otf)
			 */
			details = g_markup_printf_escaped (_ ("%s%s\n"
			                                      "Encoding: %s\n"
			                                      "%s\n"
			                                      "Substituting with <b>%s</b>\n"
			                                      "(%s)"),
			                                   type_str, standard_str,
			                                   encoding, embedded,
			                                   substitute, filename);
		else
			/* Translators: string is a concatenation of previous
			 * translated strings to indicate the fonts properties
			 * in a PDF document.
			 *
			 * Example:
			 * TrueType (CID)
			 * Encoding: Custom
			 * Embedded subset
			 */
			details = g_markup_printf_escaped (_ ("%s%s\n"
			                                      "Encoding: %s\n"
			                                      "%s"),
			                                   type_str, standard_str,
			                                   encoding, embedded);

		g_list_store_append (model,
		                     g_object_new (PPS_TYPE_FONT_DESCRIPTION,
		                                   "name", name,
		                                   "details", details,
		                                   NULL));

		g_free (details);
	} while (poppler_fonts_iter_next (iter));

	return G_LIST_MODEL (model);
}

static void
pdf_document_document_fonts_iface_init (PpsDocumentFontsInterface *iface)
{
	iface->get_model = pdf_document_fonts_get_model;
	iface->get_fonts_summary = pdf_document_fonts_get_fonts_summary;
	iface->scan = pdf_document_fonts_scan;
}

static gboolean
pdf_document_links_has_document_links (PpsDocumentLinks *document_links)
{
	PdfDocument *self = PDF_DOCUMENT (document_links);
	PopplerIndexIter *iter;

	g_return_val_if_fail (PDF_IS_DOCUMENT (document_links), FALSE);

	iter = poppler_index_iter_new (self->document);
	if (iter == NULL)
		return FALSE;
	poppler_index_iter_free (iter);

	return TRUE;
}

static PpsLinkDest *
pps_link_dest_from_dest (PdfDocument *self,
                         PopplerDest *dest)
{
	PpsLinkDest *pps_dest = NULL;
	const char *unimplemented_dest = NULL;

	g_assert (dest != NULL);

	switch (dest->type) {
	case POPPLER_DEST_XYZ: {
		PopplerPage *poppler_page;
		double height;

		poppler_page = poppler_document_get_page (self->document,
		                                          MAX (0, dest->page_num - 1));
		poppler_page_get_size (poppler_page, NULL, &height);
		pps_dest = pps_link_dest_new_xyz (dest->page_num - 1,
		                                  dest->left,
		                                  height - MIN (height, dest->top),
		                                  dest->zoom,
		                                  dest->change_left,
		                                  dest->change_top,
		                                  dest->change_zoom);
		g_object_unref (poppler_page);
	} break;
	case POPPLER_DEST_FITB:
	case POPPLER_DEST_FIT:
		pps_dest = pps_link_dest_new_fit (dest->page_num - 1);
		break;
	case POPPLER_DEST_FITBH:
	case POPPLER_DEST_FITH: {
		PopplerPage *poppler_page;
		double height;

		poppler_page = poppler_document_get_page (self->document,
		                                          MAX (0, dest->page_num - 1));
		poppler_page_get_size (poppler_page, NULL, &height);
		pps_dest = pps_link_dest_new_fith (dest->page_num - 1,
		                                   height - MIN (height, dest->top),
		                                   dest->change_top);
		g_object_unref (poppler_page);
	} break;
	case POPPLER_DEST_FITBV:
	case POPPLER_DEST_FITV:
		pps_dest = pps_link_dest_new_fitv (dest->page_num - 1,
		                                   dest->left,
		                                   dest->change_left);
		break;
	case POPPLER_DEST_FITR: {
		PopplerPage *poppler_page;
		double height;

		poppler_page = poppler_document_get_page (self->document,
		                                          MAX (0, dest->page_num - 1));
		poppler_page_get_size (poppler_page, NULL, &height);
		/* for papers we ensure that bottom <= top and left <= right */
		/* also papers has its origin in the top left, so we invert the y axis. */
		pps_dest = pps_link_dest_new_fitr (dest->page_num - 1,
		                                   MIN (dest->left, dest->right),
		                                   height - MIN (height, MIN (dest->bottom, dest->top)),
		                                   MAX (dest->left, dest->right),
		                                   height - MIN (height, MAX (dest->bottom, dest->top)));
		g_object_unref (poppler_page);
	} break;
	case POPPLER_DEST_NAMED:
		pps_dest = pps_link_dest_new_named (dest->named_dest);
		break;
	case POPPLER_DEST_UNKNOWN:
		unimplemented_dest = "POPPLER_DEST_UNKNOWN";
		break;
	}

	if (unimplemented_dest) {
		g_warning ("Unimplemented destination: %s, please post a "
		           "bug report in Papers issue tracker "
		           "(https://gitlab.gnome.org/GNOME/Incubator/papers/issues) with a testcase.",
		           unimplemented_dest);
	}

	if (!pps_dest)
		pps_dest = pps_link_dest_new_page (dest->page_num - 1);

	return pps_dest;
}

static PpsLink *
pps_link_from_action (PdfDocument *self,
                      PopplerAction *action)
{
	PpsLink *link = NULL;
	PpsLinkAction *pps_action = NULL;
	const char *unimplemented_action = NULL;

	switch (action->type) {
	case POPPLER_ACTION_NONE:
		break;
	case POPPLER_ACTION_GOTO_DEST: {
		PpsLinkDest *dest;

		dest = pps_link_dest_from_dest (self, action->goto_dest.dest);
		pps_action = pps_link_action_new_dest (dest);
		g_object_unref (dest);
	} break;
	case POPPLER_ACTION_GOTO_REMOTE: {
		PpsLinkDest *dest;

		dest = pps_link_dest_from_dest (self, action->goto_remote.dest);
		pps_action = pps_link_action_new_remote (dest,
		                                         action->goto_remote.file_name);
		g_object_unref (dest);
	} break;
	case POPPLER_ACTION_LAUNCH:
		pps_action = pps_link_action_new_launch (action->launch.file_name,
		                                         action->launch.params);
		break;
	case POPPLER_ACTION_URI:
		pps_action = pps_link_action_new_external_uri (action->uri.uri);
		break;
	case POPPLER_ACTION_NAMED:
		pps_action = pps_link_action_new_named (action->named.named_dest);
		break;
	case POPPLER_ACTION_MOVIE:
		unimplemented_action = "POPPLER_ACTION_MOVIE";
		break;
	case POPPLER_ACTION_RENDITION:
		unimplemented_action = "POPPLER_ACTION_RENDITION";
		break;
	case POPPLER_ACTION_OCG_STATE: {
		GList *on_list = NULL;
		GList *off_list = NULL;
		GList *toggle_list = NULL;
		GList *l, *m;

		for (l = action->ocg_state.state_list; l; l = g_list_next (l)) {
			PopplerActionLayer *action_layer = (PopplerActionLayer *) l->data;

			for (m = action_layer->layers; m; m = g_list_next (m)) {
				PopplerLayer *layer = (PopplerLayer *) m->data;
				PpsLayer *pps_layer;

				pps_layer = pps_layer_new (poppler_layer_get_radio_button_group_id (layer));
				g_object_set_data_full (G_OBJECT (pps_layer),
				                        "poppler-layer",
				                        g_object_ref (layer),
				                        (GDestroyNotify) g_object_unref);

				switch (action_layer->action) {
				case POPPLER_ACTION_LAYER_ON:
					on_list = g_list_prepend (on_list, pps_layer);
					break;
				case POPPLER_ACTION_LAYER_OFF:
					off_list = g_list_prepend (off_list, pps_layer);
					break;
				case POPPLER_ACTION_LAYER_TOGGLE:
					toggle_list = g_list_prepend (toggle_list, pps_layer);
					break;
				}
			}
		}

		/* The action takes the ownership of the lists */
		pps_action = pps_link_action_new_layers_state (g_list_reverse (on_list),
		                                               g_list_reverse (off_list),
		                                               g_list_reverse (toggle_list));
	} break;
	case POPPLER_ACTION_JAVASCRIPT:
		unimplemented_action = "POPPLER_ACTION_JAVASCRIPT";
		break;
	case POPPLER_ACTION_RESET_FORM: {
		gboolean exclude_reset_fields;
		GList *reset_fields = NULL;
		GList *iter;

		for (iter = action->reset_form.fields; iter; iter = iter->next)
			reset_fields = g_list_prepend (reset_fields, g_strdup ((char *) iter->data));

		exclude_reset_fields = action->reset_form.exclude;

		/* The action takes the ownership of the list */
		pps_action = pps_link_action_new_reset_form (g_list_reverse (reset_fields),
		                                             exclude_reset_fields);
		break;
	}
	case POPPLER_ACTION_UNKNOWN:
		unimplemented_action = "POPPLER_ACTION_UNKNOWN";
	}

	if (unimplemented_action) {
		g_warning ("Unimplemented action: %s, please post a bug report "
		           "in Papers issue tracker (https://gitlab.gnome.org/GNOME/Incubator/papers/issues) "
		           "with a testcase.",
		           unimplemented_action);
	}

	link = pps_link_new (action->any.title, pps_action);
	if (pps_action)
		g_object_unref (pps_action);

	return link;
}

static void
build_tree (PdfDocument *self,
            GListStore *model,
            PopplerIndexIter *iter)
{

	do {
		PopplerIndexIter *child;
		PopplerAction *action;
		PpsOutlines *outlines;
		GListStore *children = NULL;
		PpsLink *link = NULL;
		gboolean expand;
		char *title_markup;

		action = poppler_index_iter_get_action (iter);
		expand = poppler_index_iter_is_open (iter);

		if (!action)
			continue;

		link = pps_link_from_action (self, action);
		if (!link || strlen (pps_link_get_title (link)) <= 0) {
			poppler_action_free (action);
			if (link)
				g_object_unref (link);

			continue;
		}

		title_markup = g_markup_escape_text (pps_link_get_title (link), -1);

		outlines = g_object_new (PPS_TYPE_OUTLINES, "markup", title_markup, "expand", expand, "link", link, NULL);

		g_list_store_append (model, outlines);

		g_free (title_markup);
		g_object_unref (link);

		child = poppler_index_iter_get_child (iter);
		if (child) {
			children = g_list_store_new (PPS_TYPE_OUTLINES);
			build_tree (self, children, child);
		}

		g_object_set (outlines, "children", children, NULL);
		poppler_index_iter_free (child);
		poppler_action_free (action);
	} while (poppler_index_iter_next (iter));
}

static GListModel *
pdf_document_links_get_links_model (PpsDocumentLinks *document_links)
{
	PdfDocument *self = PDF_DOCUMENT (document_links);
	GListStore *model = NULL;
	PopplerIndexIter *iter;

	g_return_val_if_fail (PDF_IS_DOCUMENT (document_links), NULL);

	iter = poppler_index_iter_new (self->document);
	/* Create the model if we have items*/
	if (iter != NULL) {
		model = g_list_store_new (PPS_TYPE_OUTLINES);

		build_tree (self, model, iter);
		poppler_index_iter_free (iter);

		return G_LIST_MODEL (model);
	}

	return NULL;
}

static PpsMappingList *
pdf_document_links_get_links (PpsDocumentLinks *document_links,
                              PpsPage *page)
{
	PdfDocument *self = PDF_DOCUMENT (document_links);
	GList *mapping_list = poppler_page_get_link_mapping (POPPLER_PAGE (page->backend_page));
	GList *retval = NULL;
	GList *list;

	for (list = mapping_list; list; list = list->next) {
		PopplerLinkMapping *link_mapping;
		PpsMapping *pps_link_mapping;

		link_mapping = (PopplerLinkMapping *) list->data;
		pps_link_mapping = g_new (PpsMapping, 1);
		pps_link_mapping->data = pps_link_from_action (self,
		                                               link_mapping->action);
		pps_link_mapping->area = poppler_rect_to_pps (page, &link_mapping->area);

		retval = g_list_prepend (retval, pps_link_mapping);
	}

	poppler_page_free_link_mapping (mapping_list);

	return pps_mapping_list_new (page->index, g_list_reverse (retval), (GDestroyNotify) g_object_unref);
}

static PpsLinkDest *
pdf_document_links_find_link_dest (PpsDocumentLinks *document_links,
                                   const gchar *link_name)
{
	PdfDocument *self = PDF_DOCUMENT (document_links);
	PopplerDest *dest;
	PpsLinkDest *pps_dest = NULL;

	dest = poppler_document_find_dest (self->document,
	                                   link_name);
	if (dest) {
		pps_dest = pps_link_dest_from_dest (self, dest);
		poppler_dest_free (dest);
	}

	return pps_dest;
}

static gint
pdf_document_links_find_link_page (PpsDocumentLinks *document_links,
                                   const gchar *link_name)
{
	PdfDocument *self = PDF_DOCUMENT (document_links);
	PopplerDest *dest;
	gint retval = -1;

	dest = poppler_document_find_dest (self->document,
	                                   link_name);
	if (dest) {
		retval = dest->page_num - 1;
		poppler_dest_free (dest);
	}

	return retval;
}

static void
pdf_document_document_links_iface_init (PpsDocumentLinksInterface *iface)
{
	iface->has_document_links = pdf_document_links_has_document_links;
	iface->get_links_model = pdf_document_links_get_links_model;
	iface->get_links = pdf_document_links_get_links;
	iface->find_link_dest = pdf_document_links_find_link_dest;
	iface->find_link_page = pdf_document_links_find_link_page;
}

static PpsMappingList *
pdf_document_images_get_image_mapping (PpsDocumentImages *document_images,
                                       PpsPage *page)
{
	GList *retval = NULL;
	PopplerPage *poppler_page;
	GList *mapping_list;
	GList *list;

	poppler_page = POPPLER_PAGE (page->backend_page);
	mapping_list = poppler_page_get_image_mapping (poppler_page);

	for (list = mapping_list; list; list = list->next) {
		PopplerImageMapping *image_mapping;
		PpsMapping *pps_image_mapping;

		image_mapping = (PopplerImageMapping *) list->data;

		pps_image_mapping = g_new (PpsMapping, 1);

		pps_image_mapping->data = pps_image_new (page->index, image_mapping->image_id);
		pps_image_mapping->area.x1 = image_mapping->area.x1;
		pps_image_mapping->area.y1 = image_mapping->area.y1;
		pps_image_mapping->area.x2 = image_mapping->area.x2;
		pps_image_mapping->area.y2 = image_mapping->area.y2;

		retval = g_list_prepend (retval, pps_image_mapping);
	}

	poppler_page_free_image_mapping (mapping_list);

	return pps_mapping_list_new (page->index, g_list_reverse (retval), (GDestroyNotify) g_object_unref);
}

static GdkPixbuf *
pdf_document_images_get_image (PpsDocumentImages *document_images,
                               PpsImage *image)
{
	PdfDocument *self = PDF_DOCUMENT (document_images);
	GdkPixbuf *retval = NULL;
	PopplerPage *poppler_page;
	cairo_surface_t *surface;

	poppler_page = poppler_document_get_page (self->document,
	                                          pps_image_get_page (image));

	surface = poppler_page_get_image (poppler_page, pps_image_get_id (image));
	if (surface) {
		retval = pps_document_misc_pixbuf_from_surface (surface);
		cairo_surface_destroy (surface);
	}

	g_object_unref (poppler_page);

	return retval;
}

static void
pdf_document_document_images_iface_init (PpsDocumentImagesInterface *iface)
{
	iface->get_image_mapping = pdf_document_images_get_image_mapping;
	iface->get_image = pdf_document_images_get_image;
}

static GList *
pdf_document_find_find_text (PpsDocumentFind *document_find,
                             PpsPage *page,
                             const gchar *text,
                             PpsFindOptions options)
{
	GList *matches, *l;
	PopplerPage *poppler_page;
	GList *retval = NULL;
	guint find_flags = 0;

	g_return_val_if_fail (POPPLER_IS_PAGE (page->backend_page), NULL);
	g_return_val_if_fail (text != NULL, NULL);

	poppler_page = POPPLER_PAGE (page->backend_page);

	if (options & PPS_FIND_CASE_SENSITIVE)
		find_flags |= POPPLER_FIND_CASE_SENSITIVE;
	else /* When search is not case sensitive, do also ignore diacritics
	     to broaden our search in order to match on more expected results */
		find_flags |= POPPLER_FIND_IGNORE_DIACRITICS;

	if (options & PPS_FIND_WHOLE_WORDS_ONLY)
		find_flags |= POPPLER_FIND_WHOLE_WORDS_ONLY;

	/* Allow to match on text spanning from one line to the next */
	find_flags |= POPPLER_FIND_MULTILINE;
	matches = poppler_page_find_text_with_options (poppler_page, text, (PopplerFindFlags) find_flags);
	if (!matches)
		return NULL;

	for (l = matches; l && l->data; l = g_list_next (l)) {
		PpsFindRectangle *pps_rect = pps_find_rectangle_new ();
		PpsRectangle aux;
		PopplerRectangle *rect = (PopplerRectangle *) l->data;
		aux = poppler_rect_to_pps (page, rect);
		pps_rect->x1 = aux.x1;
		pps_rect->x2 = aux.x2;
		pps_rect->y1 = aux.y1;
		pps_rect->y2 = aux.y2;
		pps_rect->next_line = poppler_rectangle_find_get_match_continued (rect);
		pps_rect->after_hyphen = pps_rect->next_line && poppler_rectangle_find_get_ignored_hyphen (rect);
		retval = g_list_prepend (retval, pps_rect);
	}

	g_clear_list (&matches, (GDestroyNotify) poppler_rectangle_free);

	return g_list_reverse (retval);
}

static PpsFindOptions
pdf_document_find_get_supported_options (PpsDocumentFind *document_find)
{
	return (PpsFindOptions) (PPS_FIND_CASE_SENSITIVE | PPS_FIND_WHOLE_WORDS_ONLY);
}

static void
pdf_document_find_iface_init (PpsDocumentFindInterface *iface)
{
	iface->find_text = pdf_document_find_find_text;
	iface->get_supported_options = pdf_document_find_get_supported_options;
}

static void
pdf_print_context_free (PdfPrintContext *ctx)
{
	if (!ctx)
		return;

#ifdef HAVE_CAIRO_PRINT
	if (ctx->cr) {
		cairo_destroy (ctx->cr);
		ctx->cr = NULL;
	}
#else
	if (ctx->ps_file) {
		poppler_ps_file_free (ctx->ps_file);
		ctx->ps_file = NULL;
	}
#endif
	g_free (ctx);
}

static void
pdf_document_file_exporter_begin (PpsFileExporter *exporter,
                                  PpsFileExporterContext *fc)
{
	PdfDocument *self = PDF_DOCUMENT (exporter);
	PdfPrintContext *ctx;
#ifdef HAVE_CAIRO_PRINT
	cairo_surface_t *surface = NULL;
#endif

	if (self->print_ctx)
		pdf_print_context_free (self->print_ctx);
	self->print_ctx = g_new0 (PdfPrintContext, 1);
	ctx = self->print_ctx;
	ctx->format = fc->format;

#ifdef HAVE_CAIRO_PRINT
	ctx->pages_per_sheet = CLAMP (fc->pages_per_sheet, 1, 16);

	ctx->paper_width = fc->paper_width;
	ctx->paper_height = fc->paper_height;

	switch (fc->pages_per_sheet) {
	default:
	case 1:
		ctx->pages_x = 1;
		ctx->pages_y = 1;
		break;
	case 2:
		ctx->pages_x = 1;
		ctx->pages_y = 2;
		break;
	case 4:
		ctx->pages_x = 2;
		ctx->pages_y = 2;
		break;
	case 6:
		ctx->pages_x = 2;
		ctx->pages_y = 3;
		break;
	case 9:
		ctx->pages_x = 3;
		ctx->pages_y = 3;
		break;
	case 16:
		ctx->pages_x = 4;
		ctx->pages_y = 4;
		break;
	}

	ctx->pages_printed = 0;

	switch (fc->format) {
	case PPS_FILE_FORMAT_PS:
#ifdef HAVE_CAIRO_PS
		surface = cairo_ps_surface_create (fc->filename, fc->paper_width, fc->paper_height);
#endif
		break;
	case PPS_FILE_FORMAT_PDF:
#ifdef HAVE_CAIRO_PDF
		surface = cairo_pdf_surface_create (fc->filename, fc->paper_width, fc->paper_height);
#endif
		break;
	default:
		g_assert_not_reached ();
	}

	ctx->cr = cairo_create (surface);
	cairo_surface_destroy (surface);

#else  /* HAVE_CAIRO_PRINT */
	if (ctx->format == PPS_FILE_FORMAT_PS) {
		ctx->ps_file = poppler_ps_file_new (self->document,
		                                    fc->filename, fc->first_page,
		                                    fc->last_page - fc->first_page + 1);
		poppler_ps_file_set_paper_size (ctx->ps_file, fc->paper_width, fc->paper_height);
		poppler_ps_file_set_duplex (ctx->ps_file, fc->duplex);
	}
#endif /* HAVE_CAIRO_PRINT */
}

static void
pdf_document_file_exporter_begin_page (PpsFileExporter *exporter)
{
	PdfDocument *self = PDF_DOCUMENT (exporter);
	PdfPrintContext *ctx = self->print_ctx;

	g_return_if_fail (self->print_ctx != NULL);

	ctx->pages_printed = 0;

#ifdef HAVE_CAIRO_PRINT
	if (ctx->paper_width > ctx->paper_height) {
		if (ctx->format == PPS_FILE_FORMAT_PS) {
			cairo_ps_surface_set_size (cairo_get_target (ctx->cr),
			                           ctx->paper_height,
			                           ctx->paper_width);
		} else if (ctx->format == PPS_FILE_FORMAT_PDF) {
			cairo_pdf_surface_set_size (cairo_get_target (ctx->cr),
			                            ctx->paper_height,
			                            ctx->paper_width);
		}
	}
#endif /* HAVE_CAIRO_PRINT */
}

static void
pdf_document_file_exporter_do_page (PpsFileExporter *exporter,
                                    PpsRenderContext *rc)
{
	PdfDocument *self = PDF_DOCUMENT (exporter);
	PdfPrintContext *ctx = self->print_ctx;
	PopplerPage *poppler_page;
#ifdef HAVE_CAIRO_PRINT
	gdouble page_width, page_height;
	gint x, y;
	gboolean rotate;
	gdouble width, height;
	gdouble pwidth, pheight;
	gdouble xscale, yscale;
#endif

	g_return_if_fail (self->print_ctx != NULL);

	poppler_page = POPPLER_PAGE (rc->page->backend_page);

#ifdef HAVE_CAIRO_PRINT
	x = (ctx->pages_printed % ctx->pages_per_sheet) % ctx->pages_x;
	y = (ctx->pages_printed % ctx->pages_per_sheet) / ctx->pages_x;
	poppler_page_get_size (poppler_page, &page_width, &page_height);

	if (page_width > page_height && page_width > ctx->paper_width) {
		rotate = TRUE;
	} else {
		rotate = FALSE;
	}

	/* Use always portrait mode and rotate when necessary */
	if (ctx->paper_width > ctx->paper_height) {
		width = ctx->paper_height;
		height = ctx->paper_width;
		rotate = !rotate;
	} else {
		width = ctx->paper_width;
		height = ctx->paper_height;
	}

	if (ctx->pages_per_sheet == 2 || ctx->pages_per_sheet == 6) {
		rotate = !rotate;
	}

	if (rotate) {
		gint tmp1;
		gdouble tmp2;

		tmp1 = x;
		x = y;
		y = tmp1;

		tmp2 = page_width;
		page_width = page_height;
		page_height = tmp2;
	}

	pwidth = width / ctx->pages_x;
	pheight = height / ctx->pages_y;

	if ((page_width > pwidth || page_height > pheight) ||
	    (page_width < pwidth && page_height < pheight)) {
		xscale = pwidth / page_width;
		yscale = pheight / page_height;

		if (yscale < xscale) {
			xscale = yscale;
		} else {
			yscale = xscale;
		}
	} else {
		xscale = yscale = 1;
	}

	/* TODO: center */

	cairo_save (ctx->cr);
	if (rotate) {
		cairo_matrix_t matrix;

		cairo_translate (ctx->cr, (2 * y + 1) * pwidth, 0);
		cairo_matrix_init (&matrix,
		                   0, 1,
		                   -1, 0,
		                   0, 0);
		cairo_transform (ctx->cr, &matrix);
	}

	cairo_translate (ctx->cr,
	                 x * (rotate ? pheight : pwidth),
	                 y * (rotate ? pwidth : pheight));
	cairo_scale (ctx->cr, xscale, yscale);

	poppler_page_render_for_printing (poppler_page, ctx->cr);

	ctx->pages_printed++;

	cairo_restore (ctx->cr);
#else  /* HAVE_CAIRO_PRINT */
	if (ctx->format == PPS_FILE_FORMAT_PS)
		poppler_page_render_to_ps (poppler_page, ctx->ps_file);
#endif /* HAVE_CAIRO_PRINT */
}

static void
pdf_document_file_exporter_end_page (PpsFileExporter *exporter)
{
	PdfDocument *self = PDF_DOCUMENT (exporter);

	g_return_if_fail (self->print_ctx != NULL);

#ifdef HAVE_CAIRO_PRINT
	cairo_show_page (self->print_ctx->cr);
#endif
}

static void
pdf_document_file_exporter_end (PpsFileExporter *exporter)
{
	PdfDocument *self = PDF_DOCUMENT (exporter);

	pdf_print_context_free (self->print_ctx);
	self->print_ctx = NULL;
}

static PpsFileExporterCapabilities
pdf_document_file_exporter_get_capabilities (PpsFileExporter *exporter)
{
	return (PpsFileExporterCapabilities) (PPS_FILE_EXPORTER_CAN_PAGE_SET |
	                                      PPS_FILE_EXPORTER_CAN_COPIES |
	                                      PPS_FILE_EXPORTER_CAN_COLLATE |
	                                      PPS_FILE_EXPORTER_CAN_REVERSE |
	                                      PPS_FILE_EXPORTER_CAN_SCALE |
#ifdef HAVE_CAIRO_PRINT
	                                      PPS_FILE_EXPORTER_CAN_NUMBER_UP |
#endif

#ifdef HAVE_CAIRO_PDF
	                                      PPS_FILE_EXPORTER_CAN_GENERATE_PDF |
#endif
	                                      PPS_FILE_EXPORTER_CAN_GENERATE_PS);
}

static void
pdf_document_file_exporter_iface_init (PpsFileExporterInterface *iface)
{
	iface->begin = pdf_document_file_exporter_begin;
	iface->begin_page = pdf_document_file_exporter_begin_page;
	iface->do_page = pdf_document_file_exporter_do_page;
	iface->end_page = pdf_document_file_exporter_end_page;
	iface->end = pdf_document_file_exporter_end;
	iface->get_capabilities = pdf_document_file_exporter_get_capabilities;
}

/* PpsDocumentPrint */
static void
pdf_document_print_print_page (PpsDocumentPrint *document,
                               PpsPage *page,
                               cairo_t *cr)
{
	poppler_page_render_for_printing (POPPLER_PAGE (page->backend_page), cr);
}

static void
pdf_document_document_print_iface_init (PpsDocumentPrintInterface *iface)
{
	iface->print_page = pdf_document_print_print_page;
}

static void
pdf_selection_render_selection (PpsSelection *selection,
                                PpsRenderContext *rc,
                                cairo_surface_t **surface,
                                PpsRectangle *points,
                                PpsRectangle *old_points,
                                PpsSelectionStyle style,
                                GdkRGBA *text,
                                GdkRGBA *base)
{
	PopplerPage *poppler_page;
	cairo_t *cr;
	PopplerColor text_color, base_color;
	double width_points, height_points;
	gint width, height;
	double xscale, yscale;

	poppler_page = POPPLER_PAGE (rc->page->backend_page);

	poppler_page_get_size (poppler_page,
	                       &width_points, &height_points);
	pps_render_context_compute_scaled_size (rc, width_points, height_points, &width, &height);

	gdk_rgba_to_poppler_color (text, &text_color);
	gdk_rgba_to_poppler_color (base, &base_color);

	if (*surface == NULL) {
		*surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
		                                       width, height);
	}

	cr = cairo_create (*surface);
	pps_render_context_compute_scales (rc, width_points, height_points, &xscale, &yscale);
	cairo_scale (cr, xscale, yscale);
	cairo_surface_set_device_offset (*surface, 0, 0);
	memset (cairo_image_surface_get_data (*surface), 0x00,
	        cairo_image_surface_get_height (*surface) *
	            cairo_image_surface_get_stride (*surface));

	poppler_page_render_selection (poppler_page,
	                               cr,
	                               (PopplerRectangle *) points,
	                               (PopplerRectangle *) old_points,
	                               (PopplerSelectionStyle) style,
	                               &text_color,
	                               &base_color);
	cairo_destroy (cr);
}

static gchar *
pdf_selection_get_selected_text (PpsSelection *selection,
                                 PpsPage *page,
                                 PpsSelectionStyle style,
                                 PpsRectangle *points)
{
	g_return_val_if_fail (POPPLER_IS_PAGE (page->backend_page), NULL);

	return poppler_page_get_selected_text (POPPLER_PAGE (page->backend_page),
	                                       (PopplerSelectionStyle) style,
	                                       (PopplerRectangle *) points);
}

static cairo_region_t *
create_region_from_poppler_region (cairo_region_t *region,
                                   gdouble xscale,
                                   gdouble yscale)
{
	int n_rects;
	cairo_region_t *retval;

	retval = cairo_region_create ();

	n_rects = cairo_region_num_rectangles (region);
	for (int i = 0; i < n_rects; i++) {
		cairo_rectangle_int_t rect;

		cairo_region_get_rectangle (region, i, &rect);
		rect.x = (int) (rect.x * xscale + 0.5);
		rect.y = (int) (rect.y * yscale + 0.5);
		rect.width = (int) (rect.width * xscale + 0.5);
		rect.height = (int) (rect.height * yscale + 0.5);
		cairo_region_union_rectangle (retval, &rect);
	}

	return retval;
}

static cairo_region_t *
pdf_selection_get_selection_region (PpsSelection *selection,
                                    PpsRenderContext *rc,
                                    PpsSelectionStyle style,
                                    PpsRectangle *points)
{
	PopplerPage *poppler_page;
	cairo_region_t *retval, *region;
	double page_width, page_height;
	double xscale, yscale;

	poppler_page = POPPLER_PAGE (rc->page->backend_page);
	region = poppler_page_get_selected_region (poppler_page,
	                                           1.0,
	                                           (PopplerSelectionStyle) style,
	                                           (PopplerRectangle *) points);

	poppler_page_get_size (poppler_page,
	                       &page_width, &page_height);
	pps_render_context_compute_scales (rc, page_width, page_height, &xscale, &yscale);
	retval = create_region_from_poppler_region (region, xscale, yscale);
	cairo_region_destroy (region);

	return retval;
}

static void
pdf_selection_iface_init (PpsSelectionInterface *iface)
{
	iface->render_selection = pdf_selection_render_selection;
	iface->get_selected_text = pdf_selection_get_selected_text;
	iface->get_selection_region = pdf_selection_get_selection_region;
}

/* PpsDocumentText */
static cairo_region_t *
pdf_document_text_get_text_mapping (PpsDocumentText *document_text,
                                    PpsPage *page)
{
	PopplerPage *poppler_page;
	PopplerRectangle points;
	cairo_region_t *retval;

	g_return_val_if_fail (POPPLER_IS_PAGE (page->backend_page), NULL);

	poppler_page = POPPLER_PAGE (page->backend_page);

	points.x1 = 0.0;
	points.y1 = 0.0;
	poppler_page_get_size (poppler_page, &(points.x2), &(points.y2));

	retval = poppler_page_get_selected_region (poppler_page, 1.0,
	                                           POPPLER_SELECTION_GLYPH,
	                                           &points);

	return retval;
}

static gchar *
pdf_document_text_get_text (PpsDocumentText *selection,
                            PpsPage *page)
{
	g_return_val_if_fail (POPPLER_IS_PAGE (page->backend_page), NULL);

	return poppler_page_get_text (POPPLER_PAGE (page->backend_page));
}

static gboolean
pdf_document_text_get_text_layout (PpsDocumentText *selection,
                                   PpsPage *page,
                                   PpsRectangle **areas,
                                   guint *n_areas)
{
	g_return_val_if_fail (POPPLER_IS_PAGE (page->backend_page), FALSE);

	return poppler_page_get_text_layout (POPPLER_PAGE (page->backend_page),
	                                     (PopplerRectangle **) areas, n_areas);
}

static gchar *
pdf_document_text_get_text_in_area (PpsDocumentText *document_text,
                                    PpsPage *page,
                                    PpsRectangle *area)
{
	g_return_val_if_fail (POPPLER_IS_PAGE (page->backend_page), NULL);

	return poppler_page_get_text_for_area (POPPLER_PAGE (page->backend_page),
	                                       (PopplerRectangle *) area);
}

static PangoAttrList *
pdf_document_text_get_text_attrs (PpsDocumentText *document_text,
                                  PpsPage *page)
{
	GList *backend_attrs_list, *l;
	PangoAttrList *attrs_list;

	g_return_val_if_fail (POPPLER_IS_PAGE (page->backend_page), NULL);

	backend_attrs_list = poppler_page_get_text_attributes (POPPLER_PAGE (page->backend_page));
	if (!backend_attrs_list)
		return NULL;

	attrs_list = pango_attr_list_new ();
	for (l = backend_attrs_list; l; l = g_list_next (l)) {
		PopplerTextAttributes *backend_attrs = (PopplerTextAttributes *) l->data;
		PangoAttribute *attr;

		if (backend_attrs->is_underlined) {
			attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
			attr->start_index = backend_attrs->start_index;
			attr->end_index = backend_attrs->end_index;
			pango_attr_list_insert (attrs_list, attr);
		}

		attr = pango_attr_foreground_new (backend_attrs->color.red,
		                                  backend_attrs->color.green,
		                                  backend_attrs->color.blue);
		attr->start_index = backend_attrs->start_index;
		attr->end_index = backend_attrs->end_index;
		pango_attr_list_insert (attrs_list, attr);

		if (backend_attrs->font_name) {
			attr = pango_attr_family_new (backend_attrs->font_name);
			attr->start_index = backend_attrs->start_index;
			attr->end_index = backend_attrs->end_index;
			pango_attr_list_insert (attrs_list, attr);
		}

		if (backend_attrs->font_size) {
			attr = pango_attr_size_new (backend_attrs->font_size * PANGO_SCALE);
			attr->start_index = backend_attrs->start_index;
			attr->end_index = backend_attrs->end_index;
			pango_attr_list_insert (attrs_list, attr);
		}
	}

	poppler_page_free_text_attributes (backend_attrs_list);

	return attrs_list;
}

static void
pdf_document_text_iface_init (PpsDocumentTextInterface *iface)
{
	iface->get_text_mapping = pdf_document_text_get_text_mapping;
	iface->get_text = pdf_document_text_get_text;
	iface->get_text_layout = pdf_document_text_get_text_layout;
	iface->get_text_in_area = pdf_document_text_get_text_in_area;
	iface->get_text_attrs = pdf_document_text_get_text_attrs;
}

/* Page Transitions */
static gdouble
pdf_document_get_page_duration (PpsDocumentTransition *trans,
                                gint page)
{
	PdfDocument *self = PDF_DOCUMENT (trans);
	PopplerPage *poppler_page;
	gdouble duration = -1;

	poppler_page = poppler_document_get_page (self->document, page);
	if (!poppler_page)
		return -1;

	duration = poppler_page_get_duration (poppler_page);
	g_object_unref (poppler_page);

	return duration;
}

static PpsTransitionEffect *
pdf_document_get_effect (PpsDocumentTransition *trans,
                         gint page)
{
	PdfDocument *self = PDF_DOCUMENT (trans);
	PopplerPage *poppler_page;
	PopplerPageTransition *page_transition;
	PpsTransitionEffect *effect;

	poppler_page = poppler_document_get_page (self->document, page);

	if (!poppler_page)
		return NULL;

	page_transition = poppler_page_get_transition (poppler_page);

	if (!page_transition) {
		g_object_unref (poppler_page);
		return NULL;
	}

	/* enums in PopplerPageTransition match the PpsTransitionEffect ones */
	effect = pps_transition_effect_new ((PpsTransitionEffectType) page_transition->type,
	                                    "alignment", page_transition->alignment,
	                                    "direction", page_transition->direction,
	                                    "duration", page_transition->duration,
	                                    "duration-real", page_transition->duration_real,
	                                    "angle", page_transition->angle,
	                                    "scale", page_transition->scale,
	                                    "rectangular", page_transition->rectangular,
	                                    NULL);

	poppler_page_transition_free (page_transition);
	g_object_unref (poppler_page);

	return effect;
}

static void
pdf_document_page_transition_iface_init (PpsDocumentTransitionInterface *iface)
{
	iface->get_page_duration = pdf_document_get_page_duration;
	iface->get_effect = pdf_document_get_effect;
}

/* Forms */
#if 0
static void
pdf_document_get_crop_box (PpsDocument  *document,
			   int          page,
			   PpsRectangle *rect)
{
	PdfDocument *self = PDF_DOCUMENT (document);;
	PopplerPage *poppler_page;
	PopplerRectangle poppler_rect;

	poppler_page = poppler_document_get_page (self->document, page);
	poppler_page_get_crop_box (poppler_page, &poppler_rect);
	rect->x1 = poppler_rect.x1;
	rect->x2 = poppler_rect.x2;
	rect->y1 = poppler_rect.y1;
	rect->y2 = poppler_rect.y2;
}
#endif

static PpsFormField *
pps_form_field_from_poppler_field (PdfDocument *self,
                                   PopplerFormField *poppler_field)
{
	PpsFormField *pps_field = NULL;
	gint id;
	gdouble font_size;
	gboolean is_read_only;
	PopplerAction *action;
	gchar *alt_ui_name = NULL;

	id = poppler_form_field_get_id (poppler_field);
	font_size = poppler_form_field_get_font_size (poppler_field);
	is_read_only = poppler_form_field_is_read_only (poppler_field);
	action = poppler_form_field_get_action (poppler_field);
	alt_ui_name = poppler_form_field_get_alternate_ui_name (poppler_field);

	switch (poppler_form_field_get_field_type (poppler_field)) {
	case POPPLER_FORM_FIELD_TEXT: {
		PpsFormFieldText *field_text;
		PpsFormFieldTextType pps_text_type = PPS_FORM_FIELD_TEXT_NORMAL;

		switch (poppler_form_field_text_get_text_type (poppler_field)) {
		case POPPLER_FORM_TEXT_NORMAL:
			pps_text_type = PPS_FORM_FIELD_TEXT_NORMAL;
			break;
		case POPPLER_FORM_TEXT_MULTILINE:
			pps_text_type = PPS_FORM_FIELD_TEXT_MULTILINE;
			break;
		case POPPLER_FORM_TEXT_FILE_SELECT:
			pps_text_type = PPS_FORM_FIELD_TEXT_FILE_SELECT;
			break;
		}

		pps_field = pps_form_field_text_new (id, pps_text_type);
		field_text = PPS_FORM_FIELD_TEXT (pps_field);

		field_text->do_spell_check = poppler_form_field_text_do_spell_check (poppler_field);
		field_text->do_scroll = poppler_form_field_text_do_scroll (poppler_field);
		field_text->is_rich_text = poppler_form_field_text_is_rich_text (poppler_field);
		field_text->is_password = poppler_form_field_text_is_password (poppler_field);
		field_text->max_len = poppler_form_field_text_get_max_len (poppler_field);
		field_text->text = poppler_form_field_text_get_text (poppler_field);
	} break;
	case POPPLER_FORM_FIELD_BUTTON: {
		PpsFormFieldButton *field_button;
		PpsFormFieldButtonType pps_button_type = PPS_FORM_FIELD_BUTTON_PUSH;

		switch (poppler_form_field_button_get_button_type (poppler_field)) {
		case POPPLER_FORM_BUTTON_PUSH:
			pps_button_type = PPS_FORM_FIELD_BUTTON_PUSH;
			break;
		case POPPLER_FORM_BUTTON_CHECK:
			pps_button_type = PPS_FORM_FIELD_BUTTON_CHECK;
			break;
		case POPPLER_FORM_BUTTON_RADIO:
			pps_button_type = PPS_FORM_FIELD_BUTTON_RADIO;
			break;
		}

		pps_field = pps_form_field_button_new (id, pps_button_type);
		field_button = PPS_FORM_FIELD_BUTTON (pps_field);

		field_button->state = poppler_form_field_button_get_state (poppler_field);
	} break;
	case POPPLER_FORM_FIELD_CHOICE: {
		PpsFormFieldChoice *field_choice;
		PpsFormFieldChoiceType pps_choice_type = PPS_FORM_FIELD_CHOICE_COMBO;

		switch (poppler_form_field_choice_get_choice_type (poppler_field)) {
		case POPPLER_FORM_CHOICE_COMBO:
			pps_choice_type = PPS_FORM_FIELD_CHOICE_COMBO;
			break;
		case POPPLER_FORM_CHOICE_LIST:
			pps_choice_type = PPS_FORM_FIELD_CHOICE_LIST;
			break;
		}

		pps_field = pps_form_field_choice_new (id, pps_choice_type);
		field_choice = PPS_FORM_FIELD_CHOICE (pps_field);

		field_choice->is_editable = poppler_form_field_choice_is_editable (poppler_field);
		field_choice->multi_select = poppler_form_field_choice_can_select_multiple (poppler_field);
		field_choice->do_spell_check = poppler_form_field_choice_do_spell_check (poppler_field);
		field_choice->commit_on_sel_change = poppler_form_field_choice_commit_on_change (poppler_field);

		/* TODO: we need poppler_form_field_choice_get_selected_items in poppler
		field_choice->selected_items = poppler_form_field_choice_get_selected_items (poppler_field);*/
		if (field_choice->is_editable)
			field_choice->text = poppler_form_field_choice_get_text (poppler_field);
	} break;
	case POPPLER_FORM_FIELD_SIGNATURE:
		/* TODO */
		pps_field = pps_form_field_signature_new (id);
		break;
	case POPPLER_FORM_FIELD_UNKNOWN:
		return NULL;
	}

	pps_field->font_size = font_size;
	pps_field->is_read_only = is_read_only;
	pps_form_field_set_alternate_name (pps_field, alt_ui_name);

	if (action)
		pps_field->activation_link = pps_link_from_action (self, action);

	return pps_field;
}

static PpsMappingList *
pdf_document_forms_get_form_fields (PpsDocumentForms *document,
                                    PpsPage *page)
{
	GList *retval = NULL;
	GList *fields;
	GList *list;

	g_return_val_if_fail (POPPLER_IS_PAGE (page->backend_page), NULL);

	fields = poppler_page_get_form_field_mapping (POPPLER_PAGE (page->backend_page));

	for (list = fields; list; list = list->next) {
		PopplerFormFieldMapping *mapping;
		PpsMapping *field_mapping;
		PpsFormField *pps_field;

		mapping = (PopplerFormFieldMapping *) list->data;

		pps_field = pps_form_field_from_poppler_field (PDF_DOCUMENT (document), mapping->field);
		if (!pps_field)
			continue;

		field_mapping = g_new0 (PpsMapping, 1);
		field_mapping->area = poppler_rect_to_pps (page, &mapping->area);
		field_mapping->data = pps_field;
		pps_field->page = PPS_PAGE (g_object_ref (page));

		g_object_set_data_full (G_OBJECT (pps_field),
		                        "poppler-field",
		                        g_object_ref (mapping->field),
		                        (GDestroyNotify) g_object_unref);

		retval = g_list_prepend (retval, field_mapping);
	}

	poppler_page_free_form_field_mapping (fields);

	return retval ? pps_mapping_list_new (page->index,
	                                      g_list_reverse (retval),
	                                      (GDestroyNotify) g_object_unref)
	              : NULL;
}

static gboolean
pdf_document_forms_document_is_modified (PpsDocumentForms *document)
{
	return PDF_DOCUMENT (document)->forms_modified;
}

static void
pdf_document_forms_reset_form (PpsDocumentForms *document,
                               PpsLinkAction *action)
{
	poppler_document_reset_form (PDF_DOCUMENT (document)->document,
	                             pps_link_action_get_reset_fields (action),
	                             pps_link_action_get_exclude_reset_fields (action));
}

static gchar *
pdf_document_forms_form_field_text_get_text (PpsDocumentForms *document,
                                             PpsFormField *field)

{
	PopplerFormField *poppler_field;
	gchar *text;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return NULL;

	text = poppler_form_field_text_get_text (poppler_field);

	return text;
}

static void
pdf_document_forms_form_field_text_set_text (PpsDocumentForms *document,
                                             PpsFormField *field,
                                             const gchar *text)
{
	PopplerFormField *poppler_field;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return;

	poppler_form_field_text_set_text (poppler_field, text);
	PDF_DOCUMENT (document)->forms_modified = TRUE;
	pps_document_set_modified (PPS_DOCUMENT (document), TRUE);
}

static void
pdf_document_forms_form_field_button_set_state (PpsDocumentForms *document,
                                                PpsFormField *field,
                                                gboolean state)
{
	PopplerFormField *poppler_field;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return;

	poppler_form_field_button_set_state (poppler_field, state);
	PDF_DOCUMENT (document)->forms_modified = TRUE;
	pps_document_set_modified (PPS_DOCUMENT (document), TRUE);
}

static gboolean
pdf_document_forms_form_field_button_get_state (PpsDocumentForms *document,
                                                PpsFormField *field)
{
	PopplerFormField *poppler_field;
	gboolean state;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return FALSE;

	state = poppler_form_field_button_get_state (poppler_field);

	return state;
}

static gchar *
pdf_document_forms_form_field_choice_get_item (PpsDocumentForms *document,
                                               PpsFormField *field,
                                               gint index)
{
	PopplerFormField *poppler_field;
	gchar *text;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return NULL;

	text = poppler_form_field_choice_get_item (poppler_field, index);

	return text;
}

static int
pdf_document_forms_form_field_choice_get_n_items (PpsDocumentForms *document,
                                                  PpsFormField *field)
{
	PopplerFormField *poppler_field;
	gint n_items;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return -1;

	n_items = poppler_form_field_choice_get_n_items (poppler_field);

	return n_items;
}

static gboolean
pdf_document_forms_form_field_choice_is_item_selected (PpsDocumentForms *document,
                                                       PpsFormField *field,
                                                       gint index)
{
	PopplerFormField *poppler_field;
	gboolean selected;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return FALSE;

	selected = poppler_form_field_choice_is_item_selected (poppler_field, index);

	return selected;
}

static void
pdf_document_forms_form_field_choice_select_item (PpsDocumentForms *document,
                                                  PpsFormField *field,
                                                  gint index)
{
	PopplerFormField *poppler_field;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return;

	poppler_form_field_choice_select_item (poppler_field, index);
	PDF_DOCUMENT (document)->forms_modified = TRUE;
	pps_document_set_modified (PPS_DOCUMENT (document), TRUE);
}

static void
pdf_document_forms_form_field_choice_toggle_item (PpsDocumentForms *document,
                                                  PpsFormField *field,
                                                  gint index)
{
	PopplerFormField *poppler_field;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return;

	poppler_form_field_choice_toggle_item (poppler_field, index);
	PDF_DOCUMENT (document)->forms_modified = TRUE;
	pps_document_set_modified (PPS_DOCUMENT (document), TRUE);
}

static void
pdf_document_forms_form_field_choice_unselect_all (PpsDocumentForms *document,
                                                   PpsFormField *field)
{
	PopplerFormField *poppler_field;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return;

	poppler_form_field_choice_unselect_all (poppler_field);
	PDF_DOCUMENT (document)->forms_modified = TRUE;
	pps_document_set_modified (PPS_DOCUMENT (document), TRUE);
}

static void
pdf_document_forms_form_field_choice_set_text (PpsDocumentForms *document,
                                               PpsFormField *field,
                                               const gchar *text)
{
	PopplerFormField *poppler_field;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return;

	poppler_form_field_choice_set_text (poppler_field, text);
	PDF_DOCUMENT (document)->forms_modified = TRUE;
	pps_document_set_modified (PPS_DOCUMENT (document), TRUE);
}

static gchar *
pdf_document_forms_form_field_choice_get_text (PpsDocumentForms *document,
                                               PpsFormField *field)
{
	PopplerFormField *poppler_field;
	gchar *text;

	poppler_field = POPPLER_FORM_FIELD (g_object_get_data (G_OBJECT (field), "poppler-field"));
	if (!poppler_field)
		return NULL;

	text = poppler_form_field_choice_get_text (poppler_field);

	return text;
}

static void
pdf_document_document_forms_iface_init (PpsDocumentFormsInterface *iface)
{
	iface->get_form_fields = pdf_document_forms_get_form_fields;
	iface->document_is_modified = pdf_document_forms_document_is_modified;
	iface->reset_form = pdf_document_forms_reset_form;
	iface->form_field_text_get_text = pdf_document_forms_form_field_text_get_text;
	iface->form_field_text_set_text = pdf_document_forms_form_field_text_set_text;
	iface->form_field_button_set_state = pdf_document_forms_form_field_button_set_state;
	iface->form_field_button_get_state = pdf_document_forms_form_field_button_get_state;
	iface->form_field_choice_get_item = pdf_document_forms_form_field_choice_get_item;
	iface->form_field_choice_get_n_items = pdf_document_forms_form_field_choice_get_n_items;
	iface->form_field_choice_is_item_selected = pdf_document_forms_form_field_choice_is_item_selected;
	iface->form_field_choice_select_item = pdf_document_forms_form_field_choice_select_item;
	iface->form_field_choice_toggle_item = pdf_document_forms_form_field_choice_toggle_item;
	iface->form_field_choice_unselect_all = pdf_document_forms_form_field_choice_unselect_all;
	iface->form_field_choice_set_text = pdf_document_forms_form_field_choice_set_text;
	iface->form_field_choice_get_text = pdf_document_forms_form_field_choice_get_text;
}

/* Annotations */

static void
poppler_annot_color_to_gdk_rgba (PopplerAnnot *poppler_annot, GdkRGBA *color)
{
	PopplerColor *poppler_color;

	poppler_color = poppler_annot_get_color (poppler_annot);
#ifdef HAVE_FREE_TEXT
	if (POPPLER_IS_ANNOT_FREE_TEXT (poppler_annot) && !poppler_color) {
		color->alpha = 0.0;
		return;
	}
#endif
	poppler_color_to_gdk_rgba (poppler_color, color);
	g_free (poppler_color);
}

static PpsAnnotationTextIcon
get_annot_text_icon (PopplerAnnotText *poppler_annot)
{
	gchar *icon = poppler_annot_text_get_icon (poppler_annot);
	PpsAnnotationTextIcon retval;

	if (!icon)
		return PPS_ANNOTATION_TEXT_ICON_UNKNOWN;

	if (strcmp (icon, POPPLER_ANNOT_TEXT_ICON_NOTE) == 0)
		retval = PPS_ANNOTATION_TEXT_ICON_NOTE;
	else if (strcmp (icon, POPPLER_ANNOT_TEXT_ICON_COMMENT) == 0)
		retval = PPS_ANNOTATION_TEXT_ICON_COMMENT;
	else if (strcmp (icon, POPPLER_ANNOT_TEXT_ICON_KEY) == 0)
		retval = PPS_ANNOTATION_TEXT_ICON_KEY;
	else if (strcmp (icon, POPPLER_ANNOT_TEXT_ICON_HELP) == 0)
		retval = PPS_ANNOTATION_TEXT_ICON_HELP;
	else if (strcmp (icon, POPPLER_ANNOT_TEXT_ICON_NEW_PARAGRAPH) == 0)
		retval = PPS_ANNOTATION_TEXT_ICON_NEW_PARAGRAPH;
	else if (strcmp (icon, POPPLER_ANNOT_TEXT_ICON_PARAGRAPH) == 0)
		retval = PPS_ANNOTATION_TEXT_ICON_PARAGRAPH;
	else if (strcmp (icon, POPPLER_ANNOT_TEXT_ICON_INSERT) == 0)
		retval = PPS_ANNOTATION_TEXT_ICON_INSERT;
	else if (strcmp (icon, POPPLER_ANNOT_TEXT_ICON_CROSS) == 0)
		retval = PPS_ANNOTATION_TEXT_ICON_CROSS;
	else if (strcmp (icon, POPPLER_ANNOT_TEXT_ICON_CIRCLE) == 0)
		retval = PPS_ANNOTATION_TEXT_ICON_CIRCLE;
	else
		retval = PPS_ANNOTATION_TEXT_ICON_UNKNOWN;

	g_free (icon);

	return retval;
}

static const gchar *
get_poppler_annot_text_icon (PpsAnnotationTextIcon icon)
{
	switch (icon) {
	case PPS_ANNOTATION_TEXT_ICON_NOTE:
		return POPPLER_ANNOT_TEXT_ICON_NOTE;
	case PPS_ANNOTATION_TEXT_ICON_COMMENT:
		return POPPLER_ANNOT_TEXT_ICON_COMMENT;
	case PPS_ANNOTATION_TEXT_ICON_KEY:
		return POPPLER_ANNOT_TEXT_ICON_KEY;
	case PPS_ANNOTATION_TEXT_ICON_HELP:
		return POPPLER_ANNOT_TEXT_ICON_HELP;
	case PPS_ANNOTATION_TEXT_ICON_NEW_PARAGRAPH:
		return POPPLER_ANNOT_TEXT_ICON_NEW_PARAGRAPH;
	case PPS_ANNOTATION_TEXT_ICON_PARAGRAPH:
		return POPPLER_ANNOT_TEXT_ICON_PARAGRAPH;
	case PPS_ANNOTATION_TEXT_ICON_INSERT:
		return POPPLER_ANNOT_TEXT_ICON_INSERT;
	case PPS_ANNOTATION_TEXT_ICON_CROSS:
		return POPPLER_ANNOT_TEXT_ICON_CROSS;
	case PPS_ANNOTATION_TEXT_ICON_CIRCLE:
		return POPPLER_ANNOT_TEXT_ICON_CIRCLE;
	case PPS_ANNOTATION_TEXT_ICON_UNKNOWN:
	default:
		return POPPLER_ANNOT_TEXT_ICON_NOTE;
	}
}

static PpsAnnotation *
pps_annot_from_poppler_annot (PopplerAnnot *poppler_annot,
                              PpsPage *page)
{
	PpsAnnotation *pps_annot = NULL;
	const gchar *unimplemented_annot = NULL;
	gboolean reported_annot = FALSE;

	switch (poppler_annot_get_annot_type (poppler_annot)) {
	case POPPLER_ANNOT_TEXT: {
		PopplerAnnotText *poppler_text;
		PpsAnnotationText *pps_annot_text;

		poppler_text = POPPLER_ANNOT_TEXT (poppler_annot);

		pps_annot = pps_annotation_text_new (page);

		pps_annot_text = PPS_ANNOTATION_TEXT (pps_annot);
		pps_annotation_text_set_is_open (pps_annot_text,
		                                 poppler_annot_text_get_is_open (poppler_text));
		pps_annotation_text_set_icon (pps_annot_text, get_annot_text_icon (poppler_text));
	} break;
	case POPPLER_ANNOT_FILE_ATTACHMENT: {
		PopplerAnnotFileAttachment *poppler_annot_attachment;
		PopplerAttachment *poppler_attachment;
		gchar *data = NULL;
		gsize size;
		GError *error = NULL;

		poppler_annot_attachment = POPPLER_ANNOT_FILE_ATTACHMENT (poppler_annot);
		poppler_attachment = poppler_annot_file_attachment_get_attachment (poppler_annot_attachment);

		if (poppler_attachment &&
		    attachment_save_to_buffer (poppler_attachment, &data, &size, &error)) {
			PpsAttachment *pps_attachment;
			GDateTime *mtime, *ctime;

			mtime = poppler_attachment_get_mtime (poppler_attachment);
			ctime = poppler_attachment_get_ctime (poppler_attachment);

			pps_attachment = pps_attachment_new (poppler_attachment->name,
			                                     poppler_attachment->description,
			                                     mtime, ctime,
			                                     size, data);
			pps_annot = pps_annotation_attachment_new (page, pps_attachment);
			g_object_unref (pps_attachment);
		} else if (error) {
			g_warning ("%s", error->message);
			g_error_free (error);
		}

		if (poppler_attachment)
			g_object_unref (poppler_attachment);
	} break;
	case POPPLER_ANNOT_HIGHLIGHT:
		pps_annot = pps_annotation_text_markup_highlight_new (page);
		break;
	case POPPLER_ANNOT_STRIKE_OUT:
		pps_annot = pps_annotation_text_markup_strike_out_new (page);
		break;
	case POPPLER_ANNOT_UNDERLINE:
		pps_annot = pps_annotation_text_markup_underline_new (page);
		break;
	case POPPLER_ANNOT_SQUIGGLY:
		pps_annot = pps_annotation_text_markup_squiggly_new (page);
		break;
	case POPPLER_ANNOT_STAMP:
		pps_annot = pps_annotation_stamp_new (page);
		break;
	case POPPLER_ANNOT_LINK:
	case POPPLER_ANNOT_WIDGET:
	case POPPLER_ANNOT_MOVIE:
		/* Ignore link, widgets and movie annots since they are already handled */
		break;
#ifdef HAVE_FREE_TEXT
	case POPPLER_ANNOT_FREE_TEXT: {
		PopplerAnnotFreeText *poppler_ft = POPPLER_ANNOT_FREE_TEXT (poppler_annot);
		PpsAnnotationFreeText *pps_annot_ft;
		g_autoptr (PangoFontDescription) font;
		g_autoptr (PopplerFontDescription) poppler_font;
		g_autoptr (PopplerColor) font_color;
		GdkRGBA font_rgba = { 0, 0, 0, 1.0 };
		gdouble border_width;

		pps_annot = pps_annotation_free_text_new (page);
		pps_annot_ft = PPS_ANNOTATION_FREE_TEXT (pps_annot);

		/* Font */
		poppler_font = poppler_annot_free_text_get_font_desc (poppler_ft);
		font = pango_font_description_new ();
		if (poppler_font) {
			pango_font_description_set_family (font, poppler_font->font_name);
			pango_font_description_set_weight (font, poppler_font->weight);
			pango_font_description_set_style (font, poppler_font->style);
			pango_font_description_set_stretch (font, poppler_font->stretch);
			pango_font_description_set_size (font, poppler_font->size_pt * PANGO_SCALE);
		} else {
			pango_font_description_set_size (font, 12 * PANGO_SCALE);
		}
		pps_annotation_free_text_set_font_description (pps_annot_ft, font);

		/* Font color */
		font_color = poppler_annot_free_text_get_font_color (poppler_ft);
		poppler_color_to_gdk_rgba (font_color, &font_rgba);
		pps_annotation_free_text_set_font_rgba (pps_annot_ft, &font_rgba);

		/* Border width. This should be generalized to other annotations in the future. */
		poppler_annot_get_border_width (poppler_annot, &border_width);
		pps_annotation_set_border_width (pps_annot, border_width);
	} break;
#endif
	case POPPLER_ANNOT_SCREEN: {
		PopplerAction *action;

		/* Ignore screen annots containing a rendition action */
		action = poppler_annot_screen_get_action (POPPLER_ANNOT_SCREEN (poppler_annot));
		if (action && action->type == POPPLER_ACTION_RENDITION)
			break;
	}
	/* Fall through */
#ifndef HAVE_FREE_TEXT
	case POPPLER_ANNOT_FREE_TEXT:
#endif
	case POPPLER_ANNOT_3D:
	case POPPLER_ANNOT_CARET:
	case POPPLER_ANNOT_LINE:
	case POPPLER_ANNOT_SOUND:
	case POPPLER_ANNOT_SQUARE: {
		/* FIXME: These annotations are unimplemented, but they were already
		 * reported in Papers Bugzilla with test case.  We add a special
		 * warning to let the user know it is unimplemented, yet we do not
		 * want more duplicates of known issues.
		 */
		GEnumValue *enum_value;
		reported_annot = TRUE;

		enum_value = g_enum_get_value ((GEnumClass *) g_type_class_ref (POPPLER_TYPE_ANNOT_TYPE),
		                               poppler_annot_get_annot_type (poppler_annot));
		unimplemented_annot = enum_value ? enum_value->value_name : "Unknown annotation";
	} break;
	default: {
		GEnumValue *enum_value;
		reported_annot = FALSE;

		enum_value = g_enum_get_value ((GEnumClass *) g_type_class_ref (POPPLER_TYPE_ANNOT_TYPE),
		                               poppler_annot_get_annot_type (poppler_annot));
		unimplemented_annot = enum_value ? enum_value->value_name : "Unknown annotation";
	}
	}

	if (unimplemented_annot) {
		if (reported_annot) {
			g_warning ("Unimplemented annotation: %s.  It is a known issue "
			           "and it might be implemented in the future.",
			           unimplemented_annot);
		} else {
			g_warning ("Unimplemented annotation: %s, please post a "
			           "bug report in Papers issue tracker "
			           "(https://gitlab.gnome.org/GNOME/Incubator/papers/issues) with a testcase.",
			           unimplemented_annot);
		}
	}

	if (pps_annot) {
		time_t utime;
		g_autofree gchar *modified;
		g_autofree gchar *contents;
		g_autofree gchar *name;
		GdkRGBA color;

		contents = poppler_annot_get_contents (poppler_annot);
		if (contents)
			pps_annotation_set_contents (pps_annot, contents);

		name = poppler_annot_get_name (poppler_annot);
		if (name)
			pps_annotation_set_name (pps_annot, name);

		modified = poppler_annot_get_modified (poppler_annot);
		if (poppler_date_parse (modified, &utime))
			pps_annotation_set_modified_from_time_t (pps_annot, utime);
		else
			pps_annotation_set_modified (pps_annot, modified);

		poppler_annot_color_to_gdk_rgba (poppler_annot, &color);
		pps_annotation_set_rgba (pps_annot, &color);

		PopplerAnnotFlag flag = poppler_annot_get_flags (poppler_annot);
		if (flag & POPPLER_ANNOT_FLAG_HIDDEN) {
			pps_annotation_set_hidden (pps_annot, TRUE);
		} else {
			pps_annotation_set_hidden (pps_annot, FALSE);
		}

		if (PPS_IS_ANNOTATION_MARKUP (pps_annot) &&
		    pps_annotation_markup_can_have_popup (PPS_ANNOTATION_MARKUP (pps_annot)) &&
		    /*
		     * Per PDF 32000-1:2008 Table 169 stamp annotations are
		     * markup annotations, but Poppler does not subclass
		     * PopplerAnnotMarkup for PopplerAnnotStamp.
		     *
		     * This can be dropped once our minimum required version of
		     * Poppler contains these or equivalent changes:
		     * https://gitlab.freedesktop.org/poppler/poppler/-/merge_requests/1760
		     */
		    POPPLER_IS_ANNOT_MARKUP (poppler_annot)) {
			PopplerAnnotMarkup *markup;
			gchar *label;
			gdouble opacity;
			PopplerRectangle poppler_rect;

			markup = POPPLER_ANNOT_MARKUP (poppler_annot);

			if (poppler_annot_markup_get_popup_rectangle (markup, &poppler_rect)) {
				PpsRectangle pps_rect;
				gboolean is_open;

				pps_rect = poppler_rect_to_pps (page, &poppler_rect);
				is_open = poppler_annot_markup_get_popup_is_open (markup);

				g_object_set (pps_annot,
				              "rectangle", &pps_rect,
				              "popup_is_open", is_open,
				              "has_popup", TRUE,
				              NULL);
			} else {
				g_object_set (pps_annot,
				              "has_popup", FALSE,
				              NULL);
			}

			label = poppler_annot_markup_get_label (markup);
			if (label)
				g_object_set (pps_annot, "label", label, NULL);
			opacity = poppler_annot_markup_get_opacity (markup);

			g_object_set (pps_annot,
			              "opacity", opacity,
			              NULL);

			g_free (label);
		}
	}

	return pps_annot;
}

static void
annot_set_unique_name (PpsAnnotation *annot)
{
	gchar *name;

	name = g_strdup_printf ("annot-%" G_GUINT64_FORMAT, g_get_real_time ());
	pps_annotation_set_name (annot, name);
	g_free (name);
}

static void
annot_area_changed_cb (PpsAnnotation *annot,
                       GParamSpec *spec,
                       PpsMapping *mapping)
{
	pps_annotation_get_area (annot, &mapping->area);
}

static PpsMappingList *
pdf_document_annotations_get_annotations_mapping (PpsDocumentAnnotations *document_annotations,
                                                  PpsPage *page)
{
	GList *retval = NULL;
	PdfDocument *self = PDF_DOCUMENT (document_annotations);
	PpsMappingList *mapping_list;
	GList *annots;
	GList *list;

	mapping_list = (PpsMappingList *) g_hash_table_lookup (self->annots,
	                                                       GINT_TO_POINTER (page->index));
	if (mapping_list)
		return pps_mapping_list_ref (mapping_list);

	annots = poppler_page_get_annot_mapping (POPPLER_PAGE (page->backend_page));

	for (list = annots; list; list = list->next) {
		PopplerAnnotMapping *mapping;
		PpsMapping *annot_mapping;
		PpsAnnotation *pps_annot;

		mapping = (PopplerAnnotMapping *) list->data;

		pps_annot = pps_annot_from_poppler_annot (mapping->annot, page);
		if (!pps_annot)
			continue;

		/* Make sure annot has a unique name */
		if (!pps_annotation_get_name (pps_annot))
			annot_set_unique_name (pps_annot);

		annot_mapping = g_new (PpsMapping, 1);
		annot_mapping->area = poppler_rect_to_pps (page, &mapping->area);
		if (PPS_IS_ANNOTATION_TEXT (pps_annot)) {
			/* Force 24x24 rectangle */
			annot_mapping->area.x2 = annot_mapping->area.x1 + 24;
			annot_mapping->area.y2 = annot_mapping->area.y1 + 24;
		}
		annot_mapping->data = pps_annot;
		pps_annotation_set_area (pps_annot, &annot_mapping->area);
		g_signal_connect (pps_annot, "notify::area",
		                  G_CALLBACK (annot_area_changed_cb),
		                  annot_mapping);

		g_object_set_data_full (G_OBJECT (pps_annot),
		                        "poppler-annot",
		                        g_object_ref (mapping->annot),
		                        (GDestroyNotify) g_object_unref);

		retval = g_list_prepend (retval, annot_mapping);
	}

	poppler_page_free_annot_mapping (annots);

	if (!retval)
		return NULL;

	mapping_list = pps_mapping_list_new (page->index, g_list_reverse (retval), (GDestroyNotify) g_object_unref);
	g_hash_table_insert (self->annots,
	                     GINT_TO_POINTER (page->index),
	                     pps_mapping_list_ref (mapping_list));

	return mapping_list;
}

static GList *
pdf_document_annotations_get_annotations (PpsDocumentAnnotations *document_annotations,
                                          PpsPage *page)
{
	PpsMappingList *mapping_list;
	GList *annots = NULL;

	mapping_list = pdf_document_annotations_get_annotations_mapping (document_annotations,

	                                                                 page);

	if (!mapping_list)
		return NULL;

	for (GList *l = pps_mapping_list_get_list (mapping_list); l; l = l->next) {
		PpsMapping *mapping = l->data;
		PpsAnnotation *annot = mapping->data;

		annots = g_list_prepend (annots, annot);
	}

	pps_mapping_list_unref (mapping_list);
	annots = g_list_reverse (annots);

	return annots;
}

static gboolean
pdf_document_annotations_document_is_modified (PpsDocumentAnnotations *document_annotations)
{
	return PDF_DOCUMENT (document_annotations)->annots_modified;
}

static void
pdf_document_annotations_remove_annotation (PpsDocumentAnnotations *document_annotations,
                                            PpsAnnotation *annot)
{
	PdfDocument *self = PDF_DOCUMENT (document_annotations);
	PpsPage *page = pps_annotation_get_page (annot);
	PopplerPage *poppler_page = POPPLER_PAGE (page->backend_page);
	PopplerAnnot *poppler_annot;
	PpsMappingList *mapping_list;
	PpsMapping *annot_mapping;

	poppler_annot = POPPLER_ANNOT (g_object_get_data (G_OBJECT (annot), "poppler-annot"));

	poppler_page_remove_annot (poppler_page, poppler_annot);

	/* We don't check for self->annots, if it were NULL then something is really wrong */
	mapping_list = (PpsMappingList *) g_hash_table_lookup (self->annots,
	                                                       GINT_TO_POINTER (page->index));
	if (mapping_list) {
		annot_mapping = pps_mapping_list_find (mapping_list, annot);
		pps_mapping_list_remove (mapping_list, annot_mapping);
		if (pps_mapping_list_length (mapping_list) == 0)
			g_hash_table_remove (self->annots, GINT_TO_POINTER (page->index));
	}

	self->annots_modified = TRUE;
	pps_document_set_modified (PPS_DOCUMENT (document_annotations), TRUE);
}

static GArray *
get_quads_for_area (PopplerPage *page,
                    PopplerRectangle *area,
                    PopplerRectangle *bbox)
{
	guint n_rects;
	guint i, lines = 0;
	GArray *quads;
	gdouble width, height;
	PopplerRectangle *rects = NULL, *r = NULL;
	GList *l_rects = NULL, *list;
	PopplerRectangle doc_area;

	if (bbox) {
		bbox->x1 = G_MAXDOUBLE;
		bbox->y1 = G_MAXDOUBLE;
		bbox->x2 = G_MINDOUBLE;
		bbox->y2 = G_MINDOUBLE;
	}

	poppler_page_get_size (page, &width, &height);

	doc_area.x1 = area->x1;
	doc_area.x2 = area->x2;
	doc_area.y1 = height - area->y2;
	doc_area.y2 = height - area->y1;

	if (!poppler_page_get_text_layout_for_area (page, &doc_area, &rects, &n_rects))
		return NULL;

	r = g_slice_new (PopplerRectangle);
	r->x1 = G_MAXDOUBLE;
	r->y1 = G_MAXDOUBLE;
	r->x2 = G_MINDOUBLE;
	r->y2 = G_MINDOUBLE;

	for (i = 0; i < n_rects; i++) {
		gdouble max_height = MAX (rects[i].y2 - rects[i].y1, r->y2 - r->y1);
		gdouble threshhold = max_height * 0.33;

		gboolean aligned = rects[i].y2 < r->y2 + threshhold &&
		                   rects[i].y1 > r->y1 - threshhold;

		if (!aligned || ABS (r->x2 - rects[i].x1) > 0.01 * width) {
			if (i > 0)
				l_rects = g_list_append (l_rects, r);

			r = g_slice_new (PopplerRectangle);
			r->x1 = rects[i].x1;
			r->y1 = rects[i].y1;
			r->x2 = rects[i].x2;
			r->y2 = rects[i].y2;
			lines++;
		} else {
			r->x1 = MIN (r->x1, rects[i].x1);
			r->y1 = MIN (r->y1, rects[i].y1);
			r->x2 = MAX (r->x2, rects[i].x2);
			r->y2 = MAX (r->y2, rects[i].y2);
		}
	}

	l_rects = g_list_append (l_rects, r);
	l_rects = g_list_reverse (l_rects);

	quads = g_array_sized_new (TRUE, TRUE, sizeof (PopplerQuadrilateral), lines);
	g_array_set_size (quads, lines);

	for (list = l_rects, i = 0; list; list = list->next, i++) {
		PopplerQuadrilateral *quad;

		quad = &g_array_index (quads, PopplerQuadrilateral, i);
		r = (PopplerRectangle *) list->data;

		quad->p1.x = r->x1;
		quad->p1.y = height - r->y1;
		quad->p2.x = r->x2;
		quad->p2.y = height - r->y1;
		quad->p3.x = r->x1;
		quad->p3.y = height - r->y2;
		quad->p4.x = r->x2;
		quad->p4.y = height - r->y2;

		if (bbox) {
			bbox->x2 = MAX (bbox->x2, MAX (quad->p1.x, MAX (quad->p2.x, MAX (quad->p3.x, quad->p4.x))));
			bbox->y2 = MAX (bbox->y2, MAX (quad->p1.y, MAX (quad->p2.y, MAX (quad->p3.y, quad->p4.y))));
			bbox->x1 = MIN (bbox->x1, MIN (quad->p1.x, MIN (quad->p2.x, MIN (quad->p3.x, quad->p4.x))));
			bbox->y1 = MIN (bbox->y1, MIN (quad->p1.y, MIN (quad->p2.y, MIN (quad->p3.y, quad->p4.y))));
		}

		g_slice_free (PopplerRectangle, r);
	}

	g_free (rects);
	g_list_free (l_rects);

	return quads;
}

static void
pdf_document_annotations_add_annotation (PpsDocumentAnnotations *document_annotations,
                                         PpsAnnotation *annot)
{
	PdfDocument *self = PDF_DOCUMENT (document_annotations);
	PpsPage *page = pps_annotation_get_page (annot);
	PopplerAnnot *poppler_annot;
	GList *list = NULL;
	PpsMappingList *mapping_list;
	PpsMapping *annot_mapping;
	PopplerRectangle poppler_rect;
	PopplerColor poppler_color;
	GdkRGBA color;
	PpsRectangle rect;

	pps_annotation_get_area (annot, &rect);
	poppler_rect = pps_rect_to_poppler (page, &rect);

	switch (pps_annotation_get_annotation_type (annot)) {
	case PPS_ANNOTATION_TYPE_TEXT: {
		PpsAnnotationText *text = PPS_ANNOTATION_TEXT (annot);
		PpsAnnotationTextIcon icon;

		poppler_annot = poppler_annot_text_new (self->document, &poppler_rect);

		icon = pps_annotation_text_get_icon (text);
		poppler_annot_text_set_icon (POPPLER_ANNOT_TEXT (poppler_annot),
		                             get_poppler_annot_text_icon (icon));
	} break;
#ifdef HAVE_FREE_TEXT
	case PPS_ANNOTATION_TYPE_FREE_TEXT: {
		PpsAnnotationFreeText *annot_ft = PPS_ANNOTATION_FREE_TEXT (annot);
		g_autoptr (PangoFontDescription) font;
		g_autoptr (PopplerFontDescription) poppler_font;
		gdouble size;
		poppler_annot = poppler_annot_free_text_new (self->document, &poppler_rect);

		/* Fonts */
		font = pps_annotation_free_text_get_font_description (annot_ft);
		size = pango_font_description_get_size (font) / PANGO_SCALE;
		poppler_font = poppler_font_description_new (pango_font_description_get_family (font));
		poppler_font->weight = pango_font_description_get_weight (font);
		poppler_font->stretch = pango_font_description_get_stretch (font);
		poppler_font->style = pango_font_description_get_style (font);
		poppler_font->size_pt = size;
		poppler_annot_free_text_set_font_desc (POPPLER_ANNOT_FREE_TEXT (poppler_annot), poppler_font);

		poppler_annot_set_border_width (poppler_annot, pps_annotation_get_border_width (annot));
	} break;
#endif
	case PPS_ANNOTATION_TYPE_TEXT_MARKUP: {
		GArray *quads;
		PopplerRectangle bbox;

		quads = get_quads_for_area (POPPLER_PAGE (page->backend_page),
		                            &poppler_rect, &bbox);

		if (!quads)
			return;

		rect = poppler_rect_to_pps (page, &bbox);

		pps_annotation_set_area (annot, &rect);

		switch (pps_annotation_text_markup_get_markup_type (PPS_ANNOTATION_TEXT_MARKUP (annot))) {
		case PPS_ANNOTATION_TEXT_MARKUP_HIGHLIGHT:
			poppler_annot = poppler_annot_text_markup_new_highlight (self->document, &bbox, quads);
			break;
		case PPS_ANNOTATION_TEXT_MARKUP_SQUIGGLY:
			poppler_annot = poppler_annot_text_markup_new_squiggly (self->document, &bbox, quads);
			break;
		case PPS_ANNOTATION_TEXT_MARKUP_STRIKE_OUT:
			poppler_annot = poppler_annot_text_markup_new_strikeout (self->document, &bbox, quads);
			break;
		case PPS_ANNOTATION_TEXT_MARKUP_UNDERLINE:
			poppler_annot = poppler_annot_text_markup_new_underline (self->document, &bbox, quads);
			break;
		default:
			g_assert_not_reached ();
		}
		g_array_unref (quads);
	} break;
	case PPS_ANNOTATION_TYPE_STAMP: {
		poppler_annot = poppler_annot_stamp_new (self->document, &poppler_rect);
		poppler_annot_stamp_set_custom_image (POPPLER_ANNOT_STAMP (poppler_annot), pps_annotation_stamp_get_surface (PPS_ANNOTATION_STAMP (annot)), NULL);
		break;
	}
	default:
		g_assert_not_reached ();
	}

	pps_annotation_get_rgba (annot, &color);
	if (color.alpha > 0.) {
		gdk_rgba_to_poppler_color (&color, &poppler_color);
		poppler_annot_set_color (poppler_annot, &poppler_color);
	} else {
		poppler_annot_set_color (poppler_annot, NULL);
	}

	if (PPS_IS_ANNOTATION_MARKUP (annot)) {
		PpsAnnotationMarkup *markup = PPS_ANNOTATION_MARKUP (annot);
		const gchar *label;

		if (pps_annotation_markup_has_popup (markup)) {
			PpsRectangle popup_rect;

			pps_annotation_markup_get_rectangle (markup, &popup_rect);
			poppler_rect = pps_rect_to_poppler (page, &popup_rect);
			poppler_annot_markup_set_popup (POPPLER_ANNOT_MARKUP (poppler_annot), &poppler_rect);
			poppler_annot_markup_set_popup_is_open (POPPLER_ANNOT_MARKUP (poppler_annot),
			                                        pps_annotation_markup_get_popup_is_open (markup));
		}

		label = pps_annotation_markup_get_label (markup);
		if (label)
			poppler_annot_markup_set_label (POPPLER_ANNOT_MARKUP (poppler_annot), label);
	}

	poppler_page_add_annot (POPPLER_PAGE (page->backend_page), poppler_annot);

	annot_mapping = g_new (PpsMapping, 1);
	annot_mapping->area = rect;
	annot_mapping->data = annot;
	g_signal_connect (annot, "notify::area",
	                  G_CALLBACK (annot_area_changed_cb),
	                  annot_mapping);
	g_object_set_data_full (G_OBJECT (annot),
	                        "poppler-annot",
	                        poppler_annot,
	                        (GDestroyNotify) g_object_unref);

	mapping_list = (PpsMappingList *) g_hash_table_lookup (self->annots,
	                                                       GINT_TO_POINTER (page->index));

	annot_set_unique_name (annot);

	if (mapping_list) {
		list = pps_mapping_list_get_list (mapping_list);
		list = g_list_append (list, annot_mapping);
	} else {
		list = g_list_append (list, annot_mapping);
		mapping_list = pps_mapping_list_new (page->index, list, (GDestroyNotify) g_object_unref);
		g_hash_table_insert (self->annots,
		                     GINT_TO_POINTER (page->index),
		                     pps_mapping_list_ref (mapping_list));
	}

	self->annots_modified = TRUE;
	pps_document_set_modified (PPS_DOCUMENT (document_annotations), TRUE);
}

/* FIXME: We could probably add this to poppler */
static void
copy_poppler_annot (PopplerAnnot *src_annot,
                    PopplerAnnot *dst_annot)
{
	char *contents;
	PopplerColor *color;

	contents = poppler_annot_get_contents (src_annot);
	poppler_annot_set_contents (dst_annot, contents);
	g_free (contents);

	poppler_annot_set_flags (dst_annot, poppler_annot_get_flags (src_annot));

	color = poppler_annot_get_color (src_annot);
	poppler_annot_set_color (dst_annot, color);
	g_free (color);

	if (POPPLER_IS_ANNOT_MARKUP (src_annot) && POPPLER_IS_ANNOT_MARKUP (dst_annot)) {
		PopplerAnnotMarkup *src_markup = POPPLER_ANNOT_MARKUP (src_annot);
		PopplerAnnotMarkup *dst_markup = POPPLER_ANNOT_MARKUP (dst_annot);
		char *label;

		label = poppler_annot_markup_get_label (src_markup);
		poppler_annot_markup_set_label (dst_markup, label);
		g_free (label);

		poppler_annot_markup_set_opacity (dst_markup, poppler_annot_markup_get_opacity (src_markup));

		if (poppler_annot_markup_has_popup (src_markup)) {
			PopplerRectangle popup_rect;

			if (poppler_annot_markup_get_popup_rectangle (src_markup, &popup_rect)) {
				poppler_annot_markup_set_popup (dst_markup, &popup_rect);
				poppler_annot_markup_set_popup_is_open (dst_markup, poppler_annot_markup_get_popup_is_open (src_markup));
			}
		}
	}
}

static void
pdf_document_annotations_save_annotation (PpsDocumentAnnotations *document_annotations,
                                          PpsAnnotation *annot,
                                          PpsAnnotationsSaveMask mask)
{
	PopplerAnnot *poppler_annot;

	poppler_annot = POPPLER_ANNOT (g_object_get_data (G_OBJECT (annot), "poppler-annot"));
	if (!poppler_annot)
		return;

	if (mask & PPS_ANNOTATIONS_SAVE_CONTENTS)
		poppler_annot_set_contents (poppler_annot,
		                            pps_annotation_get_contents (annot));

	if (mask & PPS_ANNOTATIONS_SAVE_COLOR) {
		PopplerColor color;
		GdkRGBA pps_color;

		pps_annotation_get_rgba (annot, &pps_color);
		gdk_rgba_to_poppler_color (&pps_color, &color);

		if (pps_color.alpha > 0.) {
			poppler_annot_set_color (poppler_annot, &color);
		} else {
			poppler_annot_set_color (poppler_annot, NULL);
		}
#ifdef HAVE_FREE_TEXT
		if (PPS_IS_ANNOTATION_FREE_TEXT (annot)) {
			g_autoptr (GdkRGBA) pps_font_color = NULL;
			pps_font_color = pps_annotation_free_text_get_font_rgba (PPS_ANNOTATION_FREE_TEXT (annot));
			gdk_rgba_to_poppler_color (pps_font_color, &color);

			poppler_annot_free_text_set_font_color (
			    POPPLER_ANNOT_FREE_TEXT (poppler_annot), &color);
		}
#endif
	}

	if (mask & PPS_ANNOTATIONS_SAVE_AREA && !PPS_IS_ANNOTATION_TEXT_MARKUP (annot)) {
		PpsRectangle area;
		PopplerRectangle poppler_rect;
		PpsPage *page = pps_annotation_get_page (annot);

		pps_annotation_get_area (annot, &area);
		poppler_rect = pps_rect_to_poppler (page, &area);
		poppler_annot_set_rectangle (poppler_annot, &poppler_rect);
	}

	if (mask & PPS_ANNOTATIONS_SAVE_HIDDEN) {
		PopplerAnnotFlag flag = poppler_annot_get_flags (poppler_annot);
		flag = flag & ~POPPLER_ANNOT_FLAG_HIDDEN;
		if (pps_annotation_get_hidden (annot)) {
			flag = flag | POPPLER_ANNOT_FLAG_HIDDEN;
		}
		poppler_annot_set_flags (poppler_annot, flag);
	}

	if (PPS_IS_ANNOTATION_MARKUP (annot)) {
		PpsAnnotationMarkup *pps_markup = PPS_ANNOTATION_MARKUP (annot);
		PopplerAnnotMarkup *markup = POPPLER_ANNOT_MARKUP (poppler_annot);

		if (mask & PPS_ANNOTATIONS_SAVE_LABEL)
			poppler_annot_markup_set_label (markup, pps_annotation_markup_get_label (pps_markup));
		if (mask & PPS_ANNOTATIONS_SAVE_OPACITY)
			poppler_annot_markup_set_opacity (markup, pps_annotation_markup_get_opacity (pps_markup));
		if (mask & PPS_ANNOTATIONS_SAVE_POPUP_RECT) {
			PpsPage *page;
			PpsRectangle pps_rect;
			PopplerRectangle poppler_rect;

			page = pps_annotation_get_page (annot);
			pps_annotation_markup_get_rectangle (pps_markup, &pps_rect);
			poppler_rect = pps_rect_to_poppler (page, &pps_rect);

			if (poppler_annot_markup_has_popup (markup))
				poppler_annot_markup_set_popup_rectangle (markup, &poppler_rect);
			else
				poppler_annot_markup_set_popup (markup, &poppler_rect);
		}
		if (mask & PPS_ANNOTATIONS_SAVE_POPUP_IS_OPEN)
			poppler_annot_markup_set_popup_is_open (markup, pps_annotation_markup_get_popup_is_open (pps_markup));
	}

#ifdef HAVE_FREE_TEXT
	if (mask & PPS_ANNOTATIONS_SAVE_FREE_TEXT_FONT && PPS_IS_ANNOTATION_FREE_TEXT (annot)) {
		PpsAnnotationFreeText *annot_ft = PPS_ANNOTATION_FREE_TEXT (annot);
		PopplerAnnotFreeText *poppler_annot_ft = POPPLER_ANNOT_FREE_TEXT (poppler_annot);
		g_autoptr (PangoFontDescription) font;
		g_autoptr (PopplerFontDescription) poppler_font;
		gdouble size;

		font = pps_annotation_free_text_get_font_description (annot_ft);
		size = pango_font_description_get_size (font) / PANGO_SCALE;
		poppler_font = poppler_font_description_new (pango_font_description_get_family (font));

		poppler_font->weight = pango_font_description_get_weight (font);
		poppler_font->stretch = pango_font_description_get_stretch (font);
		poppler_font->style = pango_font_description_get_style (font);
		poppler_font->size_pt = size;
		poppler_annot_free_text_set_font_desc (poppler_annot_ft, poppler_font);
	}
#endif

	if (PPS_IS_ANNOTATION_TEXT (annot)) {
		PpsAnnotationText *pps_text = PPS_ANNOTATION_TEXT (annot);
		PopplerAnnotText *text = POPPLER_ANNOT_TEXT (poppler_annot);

		if (mask & PPS_ANNOTATIONS_SAVE_TEXT_IS_OPEN) {
			poppler_annot_text_set_is_open (text,
			                                pps_annotation_text_get_is_open (pps_text));
		}
		if (mask & PPS_ANNOTATIONS_SAVE_TEXT_ICON) {
			PpsAnnotationTextIcon icon;

			icon = pps_annotation_text_get_icon (pps_text);
			poppler_annot_text_set_icon (text, get_poppler_annot_text_icon (icon));
		}
	}

	if (PPS_IS_ANNOTATION_TEXT_MARKUP (annot)) {
		PpsAnnotationTextMarkup *pps_text_markup = PPS_ANNOTATION_TEXT_MARKUP (annot);
		PopplerAnnotTextMarkup *text_markup = POPPLER_ANNOT_TEXT_MARKUP (poppler_annot);

		if (mask & PPS_ANNOTATIONS_SAVE_TEXT_MARKUP_TYPE) {
			/* In poppler every text markup annotation type is a different class */
			PdfDocument *self = PDF_DOCUMENT (document_annotations);
			GArray *quads;
			PopplerRectangle rect;
			PopplerAnnot *new_annot = NULL;
			PpsPage *page;
			PopplerPage *poppler_page;

			quads = poppler_annot_text_markup_get_quadrilaterals (text_markup);
			poppler_annot_get_rectangle (POPPLER_ANNOT (text_markup), &rect);

			switch (pps_annotation_text_markup_get_markup_type (pps_text_markup)) {
			case PPS_ANNOTATION_TEXT_MARKUP_HIGHLIGHT:
				new_annot = poppler_annot_text_markup_new_highlight (self->document, &rect, quads);
				break;
			case PPS_ANNOTATION_TEXT_MARKUP_STRIKE_OUT:
				new_annot = poppler_annot_text_markup_new_strikeout (self->document, &rect, quads);
				break;
			case PPS_ANNOTATION_TEXT_MARKUP_UNDERLINE:
				new_annot = poppler_annot_text_markup_new_underline (self->document, &rect, quads);
				break;
			case PPS_ANNOTATION_TEXT_MARKUP_SQUIGGLY:
				new_annot = poppler_annot_text_markup_new_squiggly (self->document, &rect, quads);
				break;
			}

			g_array_unref (quads);

			copy_poppler_annot (poppler_annot, new_annot);

			page = pps_annotation_get_page (annot);
			poppler_page = POPPLER_PAGE (page->backend_page);

			poppler_page_remove_annot (poppler_page, poppler_annot);
			poppler_page_add_annot (poppler_page, new_annot);
			g_object_set_data_full (G_OBJECT (annot),
			                        "poppler-annot",
			                        new_annot,
			                        (GDestroyNotify) g_object_unref);
		}

		if (mask & PPS_ANNOTATIONS_SAVE_AREA) {
			PpsRectangle rect;
			GArray *quads;
			PpsPage *page;
			PopplerRectangle poppler_rect, bbox;

			page = pps_annotation_get_page (annot);
			pps_annotation_get_area (annot, &rect);
			poppler_rect = pps_rect_to_poppler (page, &rect);

			quads = get_quads_for_area (POPPLER_PAGE (page->backend_page),
			                            &poppler_rect, &bbox);

			if (!quads)
				return;

			rect = poppler_rect_to_pps (page, &bbox);

			pps_annotation_set_area (annot, &rect);
			poppler_annot_set_rectangle (poppler_annot, &bbox);
			poppler_annot_text_markup_set_quadrilaterals (text_markup, quads);
			g_array_unref (quads);
		}
	}

	PDF_DOCUMENT (document_annotations)->annots_modified = TRUE;
	pps_document_set_modified (PPS_DOCUMENT (document_annotations), TRUE);
}

/* Creates a vector from points @p1 and @p2 and stores it on @vector */
static inline void
set_vector (PopplerPoint *p1,
            PopplerPoint *p2,
            PopplerPoint *vector)
{
	vector->x = p2->x - p1->x;
	vector->y = p2->y - p1->y;
}

/* Returns the dot product of the passed vectors @v1 and @v2 */
static inline gdouble
dot_product (PopplerPoint *v1,
             PopplerPoint *v2)
{
	return v1->x * v2->x + v1->y * v2->y;
}

/* Algorithm: https://math.stackexchange.com/a/190203
   Implementation: https://stackoverflow.com/a/37865332 */
static gboolean
point_over_poppler_quadrilateral (PopplerQuadrilateral *quad,
                                  PopplerPoint *M)
{
	PopplerPoint AB, AM, BC, BM;
	gdouble dotABAM, dotABAB, dotBCBM, dotBCBC;

	/* We interchange p3 and p4 because algorithm expects clockwise eg. p1 -> p2
	   while pdf quadpoints are defined as p1 -> p2                     p4 <- p3
	                                       p3 -> p4 (https://stackoverflow.com/q/9855814) */
	set_vector (&quad->p1, &quad->p2, &AB);
	set_vector (&quad->p1, M, &AM);
	set_vector (&quad->p2, &quad->p4, &BC);
	set_vector (&quad->p2, M, &BM);

	dotABAM = dot_product (&AB, &AM);
	dotABAB = dot_product (&AB, &AB);
	dotBCBM = dot_product (&BC, &BM);
	dotBCBC = dot_product (&BC, &BC);

	return 0 <= dotABAM && dotABAM <= dotABAB && 0 <= dotBCBM && dotBCBM <= dotBCBC;
}

static PpsAnnotationsOverMarkup
pdf_document_annotations_over_markup (PpsDocumentAnnotations *document_annotations,
                                      PpsAnnotation *annot,
                                      gdouble x,
                                      gdouble y)
{
	PpsPage *page;
	PopplerAnnot *poppler_annot;
	GArray *quads;
	PopplerPoint M;
	guint quads_len;
	gdouble height;

	poppler_annot = POPPLER_ANNOT (g_object_get_data (G_OBJECT (annot), "poppler-annot"));

	if (!poppler_annot || !POPPLER_IS_ANNOT_TEXT_MARKUP (poppler_annot))
		return PPS_ANNOTATION_OVER_MARKUP_UNKNOWN;

	quads = poppler_annot_text_markup_get_quadrilaterals (POPPLER_ANNOT_TEXT_MARKUP (poppler_annot));
	quads_len = quads->len;

	page = pps_annotation_get_page (annot);
	poppler_page_get_size (POPPLER_PAGE (page->backend_page), NULL, &height);
	M.x = x;
	M.y = height - y;

	for (guint i = 0; i < quads_len; ++i) {
		PopplerQuadrilateral *quadrilateral = &g_array_index (quads, PopplerQuadrilateral, i);

		if (point_over_poppler_quadrilateral (quadrilateral, &M)) {
			g_array_unref (quads);
			return PPS_ANNOTATION_OVER_MARKUP_YES;
		}
	}
	g_array_unref (quads);

	return PPS_ANNOTATION_OVER_MARKUP_NOT;
}

static void
pdf_document_document_annotations_iface_init (PpsDocumentAnnotationsInterface *iface)
{
	iface->get_annotations = pdf_document_annotations_get_annotations;
	iface->get_annotations_mapping = pdf_document_annotations_get_annotations_mapping;
	iface->document_is_modified = pdf_document_annotations_document_is_modified;
	iface->add_annotation = pdf_document_annotations_add_annotation;
	iface->save_annotation = pdf_document_annotations_save_annotation;
	iface->remove_annotation = pdf_document_annotations_remove_annotation;
	iface->over_markup = pdf_document_annotations_over_markup;
}

/* Media */
static GFile *
get_media_file (const gchar *filename,
                PpsDocument *document)
{
	GFile *file;

	if (g_path_is_absolute (filename)) {
		file = g_file_new_for_path (filename);
	} else if (g_strrstr (filename, "://")) {
		file = g_file_new_for_uri (filename);
	} else {
		gchar *doc_path;
		gchar *path;
		gchar *base_dir;

		doc_path = g_filename_from_uri (pps_document_get_uri (document), NULL, NULL);
		base_dir = g_path_get_dirname (doc_path);
		g_free (doc_path);

		path = g_build_filename (base_dir, filename, NULL);
		g_free (base_dir);

		file = g_file_new_for_path (path);
		g_free (path);
	}

	return file;
}

static PpsMedia *
pps_media_from_poppler_movie (PpsDocument *document,
                              PpsPage *page,
                              PopplerMovie *movie)
{
	PpsMedia *media;
	GFile *file;
	gchar *uri;

	file = get_media_file (poppler_movie_get_filename (movie), document);
	uri = g_file_get_uri (file);
	g_object_unref (file);

	media = pps_media_new_for_uri (page, uri);
	g_free (uri);
	pps_media_set_show_controls (media, poppler_movie_show_controls (movie));

	return media;
}

static void
delete_temp_file (GFile *file)
{
	g_file_delete (file, NULL, NULL);
	g_object_unref (file);
}

static gboolean
media_save_to_file_callback (const gchar *buffer,
                             gsize count,
                             gpointer data,
                             GError **error)
{
	gint fd = GPOINTER_TO_INT (data);

	return write (fd, buffer, count) == (gssize) count;
}

static PpsMedia *
pps_media_from_poppler_rendition (PpsDocument *document,
                                  PpsPage *page,
                                  PopplerMedia *poppler_media)
{
	PpsMedia *media;
	GFile *file = NULL;
	gchar *uri;
	gboolean is_temp_file = FALSE;

	if (!poppler_media)
		return NULL;

	if (poppler_media_is_embedded (poppler_media)) {
		gint fd;
		gchar *filename;

		fd = pps_mkstemp ("evmedia.XXXXXX", &filename, NULL);
		if (fd == -1)
			return NULL;

		if (poppler_media_save_to_callback (poppler_media,
		                                    media_save_to_file_callback,
		                                    GINT_TO_POINTER (fd), NULL)) {
			file = g_file_new_for_path (filename);
			is_temp_file = TRUE;
		}
		close (fd);
		g_free (filename);
	} else {
		file = get_media_file (poppler_media_get_filename (poppler_media), document);
	}

	if (!file)
		return NULL;

	uri = g_file_get_uri (file);
	media = pps_media_new_for_uri (page, uri);
	pps_media_set_show_controls (media, TRUE);
	g_free (uri);

	if (is_temp_file)
		g_object_set_data_full (G_OBJECT (media), "poppler-media-temp-file", file, (GDestroyNotify) delete_temp_file);
	else
		g_object_unref (file);

	return media;
}

static PpsMappingList *
pdf_document_media_get_media_mapping (PpsDocumentMedia *document_media,
                                      PpsPage *page)
{
	PdfDocument *self = PDF_DOCUMENT (document_media);
	GList *annots = poppler_page_get_annot_mapping (POPPLER_PAGE (page->backend_page));
	GList *retval = NULL;
	GList *list;

	for (list = annots; list; list = list->next) {
		PopplerAnnotMapping *mapping;
		PpsMapping *media_mapping;
		PpsMedia *media = NULL;

		mapping = (PopplerAnnotMapping *) list->data;

		switch (poppler_annot_get_annot_type (mapping->annot)) {
		case POPPLER_ANNOT_MOVIE: {
			PopplerAnnotMovie *poppler_annot;

			poppler_annot = POPPLER_ANNOT_MOVIE (mapping->annot);
			media = pps_media_from_poppler_movie (PPS_DOCUMENT (self), page,
			                                      poppler_annot_movie_get_movie (poppler_annot));
		} break;
		case POPPLER_ANNOT_SCREEN: {
			PopplerAction *action;

			action = poppler_annot_screen_get_action (POPPLER_ANNOT_SCREEN (mapping->annot));
			if (action && action->type == POPPLER_ACTION_RENDITION) {
				media = pps_media_from_poppler_rendition (PPS_DOCUMENT (self), page,
				                                          action->rendition.media);
			}
		} break;
		default:
			break;
		}

		if (!media)
			continue;

		media_mapping = g_new (PpsMapping, 1);

		media_mapping->data = media;
		media_mapping->area = poppler_rect_to_pps (page, &mapping->area);

		retval = g_list_prepend (retval, media_mapping);
	}

	poppler_page_free_annot_mapping (annots);

	if (!retval)
		return NULL;

	return pps_mapping_list_new (page->index, g_list_reverse (retval), (GDestroyNotify) g_object_unref);
}

static void
pdf_document_document_media_iface_init (PpsDocumentMediaInterface *iface)
{
	iface->get_media_mapping = pdf_document_media_get_media_mapping;
}

/* Attachments */
struct SaveToBufferData {
	gchar *buffer;
	gsize len, max;
};

static gboolean
attachment_save_to_buffer_callback (const gchar *buf,
                                    gsize count,
                                    gpointer user_data,
                                    GError **error)
{
	struct SaveToBufferData *sdata = (struct SaveToBufferData *) user_data;
	gchar *new_buffer;
	gsize new_max;

	if (sdata->len + count > sdata->max) {
		new_max = MAX (sdata->max * 2, sdata->len + count);
		new_buffer = (gchar *) g_realloc (sdata->buffer, new_max);

		sdata->buffer = new_buffer;
		sdata->max = new_max;
	}

	memcpy (sdata->buffer + sdata->len, buf, count);
	sdata->len += count;

	return TRUE;
}

static gboolean
attachment_save_to_buffer (PopplerAttachment *attachment,
                           gchar **buffer,
                           gsize *buffer_size,
                           GError **error)
{
	static const gint initial_max = 1024;
	struct SaveToBufferData sdata;

	*buffer = NULL;
	*buffer_size = 0;

	sdata.buffer = (gchar *) g_malloc (initial_max);
	sdata.max = initial_max;
	sdata.len = 0;

	if (!poppler_attachment_save_to_callback (attachment,
	                                          attachment_save_to_buffer_callback,
	                                          &sdata,
	                                          error)) {
		g_free (sdata.buffer);
		return FALSE;
	}

	*buffer = sdata.buffer;
	*buffer_size = sdata.len;

	return TRUE;
}

static GList *
pdf_document_attachments_get_attachments (PpsDocumentAttachments *document)
{
	PdfDocument *self = PDF_DOCUMENT (document);
	GList *list;
	GList *retval = NULL;

	list = poppler_document_get_attachments (self->document);

	while (list) {
		PopplerAttachment *attachment = (PopplerAttachment *) list->data;
		PpsAttachment *pps_attachment;
		gchar *data = NULL;
		gsize size;
		GError *error = NULL;

		if (attachment_save_to_buffer (attachment, &data, &size, &error)) {
			GDateTime *mtime, *ctime;

			mtime = poppler_attachment_get_mtime (attachment);
			ctime = poppler_attachment_get_ctime (attachment);

			pps_attachment = pps_attachment_new (attachment->name,
			                                     attachment->description,
			                                     mtime, ctime,
			                                     size, data);

			retval = g_list_prepend (retval, pps_attachment);
		} else {
			if (error) {
				g_warning ("%s", error->message);
				g_error_free (error);

				g_free (data);
			}
		}

		list = list->next;
	}

	g_clear_list (&list, g_object_unref);

	return g_list_reverse (retval);
}

static gboolean
pdf_document_attachments_has_attachments (PpsDocumentAttachments *document)
{
	PdfDocument *self = PDF_DOCUMENT (document);

	return poppler_document_has_attachments (self->document);
}

static void
pdf_document_document_attachments_iface_init (PpsDocumentAttachmentsInterface *iface)
{
	iface->has_attachments = pdf_document_attachments_has_attachments;
	iface->get_attachments = pdf_document_attachments_get_attachments;
}

/* Layers */
static gboolean
pdf_document_layers_has_layers (PpsDocumentLayers *document)
{
	PdfDocument *self = PDF_DOCUMENT (document);
	PopplerLayersIter *iter;

	iter = poppler_layers_iter_new (self->document);
	if (!iter)
		return FALSE;
	poppler_layers_iter_free (iter);

	return TRUE;
}

static void
build_layers_tree (PdfDocument *self,
                   GListStore *model,
                   PopplerLayersIter *iter)
{
	do {
		PopplerLayersIter *child;
		PopplerLayer *layer;
		PpsLayer *pps_layer = NULL;
		gboolean visible, is_title_only = FALSE;
		gchar *markup;
		gint rb_group = 0;

		layer = poppler_layers_iter_get_layer (iter);
		if (layer) {
			markup = g_markup_escape_text (poppler_layer_get_title (layer), -1);
			visible = poppler_layer_is_visible (layer);
			rb_group = poppler_layer_get_radio_button_group_id (layer);
		} else {
			gchar *title;

			title = poppler_layers_iter_get_title (iter);

			if (title == NULL)
				continue;

			markup = g_markup_escape_text (title, -1);
			g_free (title);

			visible = FALSE;
			is_title_only = TRUE;
		}

		pps_layer = pps_layer_new (rb_group);

		g_object_set (pps_layer, "title-only", is_title_only, "enabled", visible, "title", markup, NULL);

		if (layer)
			g_object_set_data_full (G_OBJECT (pps_layer),
			                        "poppler-layer",
			                        g_object_ref (layer),
			                        (GDestroyNotify) g_object_unref);

		g_list_store_append (model, pps_layer);

		g_free (markup);

		child = poppler_layers_iter_get_child (iter);

		if (child) {
			GListStore *children = g_list_store_new (PPS_TYPE_LAYER);
			build_layers_tree (self, children, child);
			pps_layer_set_children (pps_layer, G_LIST_MODEL (children));
		}

		poppler_layers_iter_free (child);
	} while (poppler_layers_iter_next (iter));
}

static GListModel *
pdf_document_layers_get_layers (PpsDocumentLayers *document)
{
	GListStore *model = NULL;
	PdfDocument *self = PDF_DOCUMENT (document);
	PopplerLayersIter *iter;

	iter = poppler_layers_iter_new (self->document);
	if (iter) {
		model = g_list_store_new (PPS_TYPE_LAYER);
		build_layers_tree (self, model, iter);
		poppler_layers_iter_free (iter);
		return G_LIST_MODEL (model);
	}

	return NULL;
}

static void
pdf_document_layers_show_layer (PpsDocumentLayers *document,
                                PpsLayer *layer)
{
	PopplerLayer *poppler_layer;

	poppler_layer = POPPLER_LAYER (g_object_get_data (G_OBJECT (layer), "poppler-layer"));
	poppler_layer_show (poppler_layer);
}

static void
pdf_document_layers_hide_layer (PpsDocumentLayers *document,
                                PpsLayer *layer)
{
	PopplerLayer *poppler_layer;

	poppler_layer = POPPLER_LAYER (g_object_get_data (G_OBJECT (layer), "poppler-layer"));
	poppler_layer_hide (poppler_layer);
}

static gboolean
pdf_document_layers_layer_is_visible (PpsDocumentLayers *document,
                                      PpsLayer *layer)
{
	PopplerLayer *poppler_layer;

	poppler_layer = POPPLER_LAYER (g_object_get_data (G_OBJECT (layer), "poppler-layer"));
	return poppler_layer_is_visible (poppler_layer);
}

static void
pdf_document_document_layers_iface_init (PpsDocumentLayersInterface *iface)
{
	iface->has_layers = pdf_document_layers_has_layers;
	iface->get_layers = pdf_document_layers_get_layers;
	iface->show_layer = pdf_document_layers_show_layer;
	iface->hide_layer = pdf_document_layers_hide_layer;
	iface->layer_is_visible = pdf_document_layers_layer_is_visible;
}

GType
pps_backend_query_type (void)
{
	return PDF_TYPE_DOCUMENT;
}

/* Signatures */

static PopplerCertificateInfo *
find_poppler_certificate_info (PpsCertificateInfo *certificate_info)
{
	g_autolist (PopplerCertificateInfo) signing_certificates = NULL;
	g_autofree char *certificate_id = NULL;

	signing_certificates = poppler_get_available_signing_certificates ();
	g_object_get (certificate_info, "id", &certificate_id, NULL);

	for (GList *list = signing_certificates; list != NULL && list->data != NULL; list = list->next) {
		PopplerCertificateInfo *certificate_info = list->data;

		if (g_strcmp0 (certificate_id, poppler_certificate_info_get_id (certificate_info)) == 0)
			return poppler_certificate_info_copy (certificate_info);
	}

	return NULL;
}

static void
poppler_sign_callback_wrapper (GObject *source_object,
                               GAsyncResult *result,
                               gpointer user_data)
{
	g_autoptr (GTask) task = user_data;
	g_autoptr (GError) error = NULL;

	if (!poppler_document_sign_finish (POPPLER_DOCUMENT (source_object), result, &error)) {
		g_task_return_error (G_TASK (result), error);
		return;
	}

	g_task_return_boolean (task, TRUE);
}

static void
pdf_document_signatures_sign (PpsDocumentSignatures *document,
                              PpsSignature *signature,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer user_data)
{
	PdfDocument *self = PDF_DOCUMENT (document);
	g_autoptr (PopplerSigningData) signing_data = poppler_signing_data_new ();
	g_autoptr (PopplerCertificateInfo) cert_info = NULL;
	g_autoptr (PpsCertificateInfo) cinfo = NULL;
	g_autoptr (GTask) task = NULL;
	PopplerRectangle signing_rect;
	PpsRectangle *rect;
	PpsPage *page;
	PopplerColor color;
	GdkRGBA rgba;

	g_object_get (signature, "certificate-info", &cinfo, NULL);
	cert_info = find_poppler_certificate_info (cinfo);
	g_assert (cert_info);

	poppler_signing_data_set_certificate_info (signing_data, cert_info);
	poppler_signing_data_set_page (signing_data, pps_signature_get_page (signature));
	poppler_signing_data_set_field_partial_name (signing_data, g_uuid_string_random ());
	poppler_signing_data_set_destination_filename (signing_data, pps_signature_get_destination_file (signature));

	if (pps_signature_get_password (signature))
		poppler_signing_data_set_password (signing_data, pps_signature_get_password (signature));

	poppler_signing_data_set_signature_text (signing_data, pps_signature_get_signature (signature));
	poppler_signing_data_set_signature_text_left (signing_data, pps_signature_get_signature_left (signature));

	pps_signature_get_font_color (signature, &rgba);
	gdk_rgba_to_poppler_color (&rgba, &color);
	poppler_signing_data_set_font_color (signing_data, &color);

	pps_signature_get_border_color (signature, &rgba);
	gdk_rgba_to_poppler_color (&rgba, &color);
	poppler_signing_data_set_border_color (signing_data, &color);

	pps_signature_get_background_color (signature, &rgba);
	gdk_rgba_to_poppler_color (&rgba, &color);
	poppler_signing_data_set_background_color (signing_data, &color);

	/* TODO: Add auto font calculation once poppler is ready */
	poppler_signing_data_set_font_size (signing_data, pps_signature_get_font_size (signature));
	poppler_signing_data_set_left_font_size (signing_data, pps_signature_get_left_font_size (signature));
	poppler_signing_data_set_border_width (signing_data, pps_signature_get_border_width (signature));

	if (pps_signature_get_owner_password (signature))
		poppler_signing_data_set_document_owner_password (signing_data, pps_signature_get_owner_password (signature));

	if (pps_signature_get_user_password (signature))
		poppler_signing_data_set_document_user_password (signing_data, pps_signature_get_user_password (signature));

	rect = pps_signature_get_rect (signature);
	page = pps_document_get_page (PPS_DOCUMENT (document),
	                              pps_signature_get_page (signature));
	signing_rect = pps_rect_to_poppler (page, rect);

	poppler_signing_data_set_signature_rectangle (signing_data, &signing_rect);

	task = g_task_new (document, cancellable, callback, user_data);

	/* Keep the signing data alive for the whole task lifetime since poppler
	 * document sign is buggy upstream and does copy it, but we still don't want
	 * to leak it here.
	 * Drop this when we depend on a poppler version that includes the MR below:
	 *   https://gitlab.freedesktop.org/poppler/poppler/-/merge_requests/1803
	 */
	g_task_set_task_data (task, signing_data,
	                      (GDestroyNotify) poppler_signing_data_free);

	poppler_document_sign (POPPLER_DOCUMENT (self->document), signing_data,
	                       cancellable, poppler_sign_callback_wrapper,
	                       g_steal_pointer (&task));

	/* Remove this when the task data workaround above is removed too */
	g_steal_pointer (&signing_data);
}

static gboolean
pdf_document_signatures_sign_finish (PpsDocumentSignatures *document_signatures,
                                     GAsyncResult *result,
                                     GError **error)
{
	return g_task_propagate_boolean (G_TASK (result), error);
}

static gboolean
pdf_document_signatures_can_sign (PpsDocumentSignatures *document)
{
	return TRUE;
}

static PpsSignaturePasswordCallback real_cb = NULL;
static gpointer cb_data = NULL;

static char *
nss_callback_wrapper (const char *name)
{
	return real_cb (name, cb_data);
}

static void
pdf_document_set_password_callback (PpsDocumentSignatures *document,
                                    PpsSignaturePasswordCallback cb,
                                    gpointer user_data)
{
	if (!cb)
		poppler_set_nss_password_callback (NULL);
	else
		poppler_set_nss_password_callback (nss_callback_wrapper);

	real_cb = cb;
	cb_data = user_data;
}

static GList *
pdf_document_get_available_signing_certificates (PpsDocumentSignatures *document)
{
	g_autolist (PopplerCertificateInfo) signing_certs = poppler_get_available_signing_certificates ();
	GList *ev_certs = NULL;

	for (GList *list = signing_certs; list != NULL && list->data != NULL; list = list->next) {
		PopplerCertificateInfo *info = list->data;
		PpsCertificateInfo *cert_info = g_object_new (PPS_TYPE_CERTIFICATE_INFO,
		                                              "id", poppler_certificate_info_get_id (info),
		                                              "subject-common-name", poppler_certificate_info_get_subject_common_name (info),
		                                              NULL);

		ev_certs = g_list_append (ev_certs, cert_info);
	}

	return ev_certs;
}

static PpsCertificateInfo *
pdf_document_get_certificate_info (PpsDocumentSignatures *document,
                                   const char *id)
{
	g_autolist (PpsCertificateInfo) list = NULL;
	PpsCertificateInfo *info = NULL;

	if (!id || strlen (id) == 0)
		return NULL;

	for (list = pdf_document_get_available_signing_certificates (document); list != NULL && list->data != NULL; list = list->next) {
		PpsCertificateInfo *cert_info = list->data;
		g_autofree char *certificate_id = NULL;

		g_object_get (cert_info, "id", &certificate_id, NULL);

		if (g_strcmp0 (certificate_id, id) == 0) {
			info = g_object_ref (cert_info);
			break;
		}
	}

	return info;
}

static GList *
pdf_document_signatures_get_signatures (PpsDocumentSignatures *document)
{
	PdfDocument *self = PDF_DOCUMENT (document);
	GList *ret_list = NULL;
	GList *signature_fields = NULL;
	GList *iter;

	signature_fields = poppler_document_get_signature_fields (self->document);

	for (iter = signature_fields; iter != NULL; iter = iter->next) {
		PopplerFormField *field = iter->data;
		PopplerSignatureInfo *info = NULL;
		PpsSignature *signature = NULL;
		PpsSignatureStatus signature_status;
		PpsCertificateStatus certificate_status;
		GDateTime *sign_time;
		PopplerCertificateInfo *certificate_info;
		PpsCertificateInfo *cert_info;

		if (poppler_form_field_get_field_type (field) != POPPLER_FORM_FIELD_SIGNATURE)
			continue;

		info = poppler_form_field_signature_validate_sync (field,
		                                                   POPPLER_SIGNATURE_VALIDATION_FLAG_VALIDATE_CERTIFICATE |
		                                                       POPPLER_SIGNATURE_VALIDATION_FLAG_USE_AIA_CERTIFICATE_FETCH,
		                                                   NULL,
		                                                   NULL);
		if (info == NULL || poppler_signature_info_get_certificate_info (info) == NULL)
			continue;

		switch (poppler_signature_info_get_signature_status (info)) {
		case POPPLER_SIGNATURE_VALID:
			signature_status = PPS_SIGNATURE_STATUS_VALID;
			break;
		case POPPLER_SIGNATURE_INVALID:
			signature_status = PPS_SIGNATURE_STATUS_INVALID;
			break;
		case POPPLER_SIGNATURE_DIGEST_MISMATCH:
			signature_status = PPS_SIGNATURE_STATUS_DIGEST_MISMATCH;
			break;
		case POPPLER_SIGNATURE_DECODING_ERROR:
			signature_status = PPS_SIGNATURE_STATUS_DECODING_ERROR;
			break;
		default:
		case POPPLER_SIGNATURE_GENERIC_ERROR:
			signature_status = PPS_SIGNATURE_STATUS_GENERIC_ERROR;
			break;
		}
		poppler_signature_info_free (info);

		info = poppler_form_field_signature_validate_sync (field, POPPLER_SIGNATURE_VALIDATION_FLAG_VALIDATE_CERTIFICATE, NULL, NULL);
		switch (poppler_signature_info_get_certificate_status (info)) {
		case POPPLER_CERTIFICATE_TRUSTED:
			certificate_status = PPS_CERTIFICATE_STATUS_TRUSTED;
			break;
		case POPPLER_CERTIFICATE_UNTRUSTED_ISSUER:
			certificate_status = PPS_CERTIFICATE_STATUS_UNTRUSTED_ISSUER;
			break;
		case POPPLER_CERTIFICATE_UNKNOWN_ISSUER:
			certificate_status = PPS_CERTIFICATE_STATUS_UNKNOWN_ISSUER;
			break;
		case POPPLER_CERTIFICATE_REVOKED:
			certificate_status = PPS_CERTIFICATE_STATUS_REVOKED;
			break;
		case POPPLER_CERTIFICATE_EXPIRED:
			certificate_status = PPS_CERTIFICATE_STATUS_EXPIRED;
			break;
		case POPPLER_CERTIFICATE_GENERIC_ERROR:
			certificate_status = PPS_CERTIFICATE_STATUS_GENERIC_ERROR;
			break;
		default:
		case POPPLER_CERTIFICATE_NOT_VERIFIED:
			certificate_status = PPS_CERTIFICATE_STATUS_NOT_VERIFIED;
			break;
		}

		certificate_info = poppler_signature_info_get_certificate_info (info);
		if (certificate_info != NULL) {
			cert_info = g_object_new (PPS_TYPE_CERTIFICATE_INFO,
			                          "subject-common-name", poppler_certificate_info_get_subject_common_name (certificate_info),
			                          "subject-email", poppler_certificate_info_get_subject_email (certificate_info),
			                          "subject-organization", poppler_certificate_info_get_subject_organization (certificate_info),
			                          "issuer-common-name", poppler_certificate_info_get_issuer_common_name (certificate_info),
			                          "issuer-email", poppler_certificate_info_get_issuer_email (certificate_info),
			                          "issuer-organization", poppler_certificate_info_get_issuer_organization (certificate_info),
			                          "issuance-time", poppler_certificate_info_get_issuance_time (certificate_info),
			                          "expiration-time", poppler_certificate_info_get_expiration_time (certificate_info),
			                          "status", certificate_status,
			                          NULL);

			sign_time = poppler_signature_info_get_local_signing_time (info);

			signature = g_object_new (PPS_TYPE_SIGNATURE,
			                          "certificate-info", cert_info,
			                          "signature-time", sign_time,
			                          "status", signature_status,
			                          NULL);

			ret_list = g_list_append (ret_list, signature);

			g_object_unref (cert_info);
		} else {
			g_warning ("Could not get certificate info for a signature!");
		}
		poppler_signature_info_free (info);
	}

	g_clear_list (&signature_fields, g_object_unref);

	return ret_list;
}

static gboolean
pdf_document_signatures_has_signatures (PpsDocumentSignatures *document)
{
	PdfDocument *pdf_document = PDF_DOCUMENT (document);

	return (poppler_document_get_n_signatures (pdf_document->document) > 0);
}

static void
pdf_document_document_signatures_iface_init (PpsDocumentSignaturesInterface *iface)
{
	iface->set_password_callback = pdf_document_set_password_callback;
	iface->get_available_signing_certificates = pdf_document_get_available_signing_certificates;
	iface->get_certificate_info = pdf_document_get_certificate_info;
	iface->sign = pdf_document_signatures_sign;
	iface->sign_finish = pdf_document_signatures_sign_finish;
	iface->can_sign = pdf_document_signatures_can_sign;
	iface->get_signatures = pdf_document_signatures_get_signatures;
	iface->has_signatures = pdf_document_signatures_has_signatures;
}
