/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
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

#pragma once

#if !defined (__PPS_PAPERS_VIEW_H_INSIDE__) && !defined (PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <cairo.h>

#include <papers-document.h>

G_BEGIN_DECLS

typedef struct _PpsJob PpsJob;
typedef struct _PpsJobClass PpsJobClass;

typedef struct _PpsJobRenderTexture PpsJobRenderTexture;
typedef struct _PpsJobRenderTextureClass PpsJobRenderTextureClass;

typedef struct _PpsJobPageData PpsJobPageData;
typedef struct _PpsJobPageDataClass PpsJobPageDataClass;

typedef struct _PpsJobThumbnailCairo PpsJobThumbnailCairo;
typedef struct _PpsJobThumbnailCairoClass PpsJobThumbnailCairoClass;

typedef struct _PpsJobThumbnailTexture PpsJobThumbnailTexture;
typedef struct _PpsJobThumbnailTextureClass PpsJobThumbnailTextureClass;

typedef struct _PpsJobLinks PpsJobLinks;
typedef struct _PpsJobLinksClass PpsJobLinksClass;

typedef struct _PpsJobAttachments PpsJobAttachments;
typedef struct _PpsJobAttachmentsClass PpsJobAttachmentsClass;

typedef struct _PpsJobAnnots PpsJobAnnots;
typedef struct _PpsJobAnnotsClass PpsJobAnnotsClass;

typedef struct _PpsJobFonts PpsJobFonts;
typedef struct _PpsJobFontsClass PpsJobFontsClass;

typedef struct _PpsJobLoad PpsJobLoad;
typedef struct _PpsJobLoadClass PpsJobLoadClass;

typedef struct _PpsJobSave PpsJobSave;
typedef struct _PpsJobSaveClass PpsJobSaveClass;

typedef struct _PpsJobFind PpsJobFind;
typedef struct _PpsJobFindClass PpsJobFindClass;

typedef struct _PpsJobLayers PpsJobLayers;
typedef struct _PpsJobLayersClass PpsJobLayersClass;

typedef struct _PpsJobExport PpsJobExport;
typedef struct _PpsJobExportClass PpsJobExportClass;

typedef struct _PpsJobPrint PpsJobPrint;
typedef struct _PpsJobPrintClass PpsJobPrintClass;

#define PPS_TYPE_JOB            (pps_job_get_type())
#define PPS_JOB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB, PpsJob))
#define PPS_IS_JOB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB))
#define PPS_JOB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB, PpsJobClass))
#define PPS_IS_JOB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB))
#define PPS_JOB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB, PpsJobClass))

#define PPS_TYPE_JOB_LINKS            (pps_job_links_get_type())
#define PPS_JOB_LINKS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_LINKS, PpsJobLinks))
#define PPS_IS_JOB_LINKS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_LINKS))
#define PPS_JOB_LINKS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_LINKS, PpsJobLinksClass))
#define PPS_IS_JOB_LINKS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_LINKS))
#define PPS_JOB_LINKS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_LINKS, PpsJobLinksClass))

#define PPS_TYPE_JOB_ATTACHMENTS           (pps_job_attachments_get_type())
#define PPS_JOB_ATTACHMENTS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_ATTACHMENTS, PpsJobAttachments))
#define PPS_IS_JOB_ATTACHMENTS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_ATTACHMENTS))
#define PPS_JOB_ATTACHMENTS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_ATTACHMENTS, PpsJobAttachmentsClass))
#define PPS_IS_JOB_ATTACHMENTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_ATTACHMENTS))
#define PPS_JOB_ATTACHMENTS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_ATTACHMENTS, PpsJobAttachmentsClass))

#define PPS_TYPE_JOB_ANNOTS            (pps_job_annots_get_type())
#define PPS_JOB_ANNOTS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_ANNOTS, PpsJobAnnots))
#define PPS_IS_JOB_ANNOTS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_ANNOTS))
#define PPS_JOB_ANNOTS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_ANNOTS, PpsJobAnnotsClass))
#define PPS_IS_JOB_ANNOTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_ANNOTS))
#define PPS_JOB_ANNOTS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_ANNOTS, PpsJobAnnotsClass))

#define PPS_TYPE_JOB_RENDER_TEXTURE            (pps_job_render_texture_get_type())
#define PPS_JOB_RENDER_TEXTURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_RENDER_TEXTURE, PpsJobRenderTexture))
#define PPS_IS_JOB_RENDER_TEXTURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_RENDER_TEXTURE))
#define PPS_JOB_RENDER_TEXTURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_RENDER_TEXTURE, PpsJobRenderTextureClass))
#define PPS_IS_JOB_RENDER_TEXTURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_RENDER_TEXTURE))
#define PPS_JOB_RENDER_TEXTURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_RENDER_TEXTURE, PpsJobRenderTextureClass))

#define PPS_TYPE_JOB_PAGE_DATA            (pps_job_page_data_get_type())
#define PPS_JOB_PAGE_DATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_PAGE_DATA, PpsJobPageData))
#define PPS_IS_JOB_PAGE_DATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_PAGE_DATA))
#define PPS_JOB_PAGE_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_PAGE_DATA, PpsJobPageDataClass))
#define PPS_IS_JOB_PAGE_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_PAGE_DATA))
#define PPS_JOB_PAGE_DATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_PAGE_DATA, PpsJobPageDataClass))

#define PPS_TYPE_JOB_THUMBNAIL_CAIRO            (pps_job_thumbnail_cairo_get_type())
#define PPS_JOB_THUMBNAIL_CAIRO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_THUMBNAIL_CAIRO, PpsJobThumbnailCairo))
#define PPS_IS_JOB_THUMBNAIL_CAIRO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_THUMBNAIL_CAIRO))
#define PPS_JOB_THUMBNAIL_CAIRO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_THUMBNAIL_CAIRO, PpsJobThumbnailCairoClass))
#define PPS_IS_JOB_THUMBNAIL_CAIRO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_THUMBNAIL_CAIRO))
#define PPS_JOB_THUMBNAIL_CAIRO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_THUMBNAIL_CAIRO, PpsJobThumbnailCairoClass))

#define PPS_TYPE_JOB_THUMBNAIL_TEXTURE            (pps_job_thumbnail_texture_get_type())
#define PPS_JOB_THUMBNAIL_TEXTURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_THUMBNAIL_TEXTURE, PpsJobThumbnailTexture))
#define PPS_IS_JOB_THUMBNAIL_TEXTURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_THUMBNAIL_TEXTURE))
#define PPS_JOB_THUMBNAIL_TEXTURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_THUMBNAIL_TEXTURE, PpsJobThumbnailTextureClass))
#define PPS_IS_JOB_THUMBNAIL_TEXTURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_THUMBNAIL_TEXTURE))
#define PPS_JOB_THUMBNAIL_TEXTURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_THUMBNAIL_TEXTURE, PpsJobThumbnailTextureClass))

#define PPS_TYPE_JOB_FONTS            (pps_job_fonts_get_type())
#define PPS_JOB_FONTS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_FONTS, PpsJobFonts))
#define PPS_IS_JOB_FONTS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_FONTS))
#define PPS_JOB_FONTS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_FONTS, PpsJobFontsClass))
#define PPS_IS_JOB_FONTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_FONTS))
#define PPS_JOB_FONTS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_FONTS, PpsJobFontsClass))


#define PPS_TYPE_JOB_LOAD            (pps_job_load_get_type())
#define PPS_JOB_LOAD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_LOAD, PpsJobLoad))
#define PPS_IS_JOB_LOAD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_LOAD))
#define PPS_JOB_LOAD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_LOAD, PpsJobLoadClass))
#define PPS_IS_JOB_LOAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_LOAD))
#define PPS_JOB_LOAD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_LOAD, PpsJobLoadClass))

#define PPS_TYPE_JOB_SAVE            (pps_job_save_get_type())
#define PPS_JOB_SAVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_SAVE, PpsJobSave))
#define PPS_IS_JOB_SAVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_SAVE))
#define PPS_JOB_SAVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_SAVE, PpsJobSaveClass))
#define PPS_IS_JOB_SAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_SAVE))
#define PPS_JOB_SAVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_SAVE, PpsJobSaveClass))

#define PPS_TYPE_JOB_FIND            (pps_job_find_get_type())
#define PPS_JOB_FIND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_FIND, PpsJobFind))
#define PPS_IS_JOB_FIND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_FIND))
#define PPS_JOB_FIND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_FIND, PpsJobFindClass))
#define PPS_IS_JOB_FIND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_FIND))
#define PPS_JOB_FIND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_FIND, PpsJobFindClass))

#define PPS_TYPE_JOB_LAYERS            (pps_job_layers_get_type())
#define PPS_JOB_LAYERS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_LAYERS, PpsJobLayers))
#define PPS_IS_JOB_LAYERS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_LAYERS))
#define PPS_JOB_LAYERS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_LAYERS, PpsJobLayersClass))
#define PPS_IS_JOB_LAYERS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_LAYERS))
#define PPS_JOB_LAYERS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_LAYERS, PpsJobLayersClass))

#define PPS_TYPE_JOB_EXPORT            (pps_job_export_get_type())
#define PPS_JOB_EXPORT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_EXPORT, PpsJobExport))
#define PPS_IS_JOB_EXPORT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_EXPORT))
#define PPS_JOB_EXPORT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_EXPORT, PpsJobExportClass))
#define PPS_IS_JOB_EXPORT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_EXPORT))
#define PPS_JOB_EXPORT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_EXPORT, PpsJobExportClass))

#define PPS_TYPE_JOB_PRINT            (pps_job_print_get_type())
#define PPS_JOB_PRINT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPS_TYPE_JOB_PRINT, PpsJobPrint))
#define PPS_IS_JOB_PRINT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPS_TYPE_JOB_PRINT))
#define PPS_JOB_PRINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PPS_TYPE_JOB_PRINT, PpsJobPrintClass))
#define PPS_IS_JOB_PRINT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PPS_TYPE_JOB_PRINT))
#define PPS_JOB_PRINT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PPS_TYPE_JOB_PRINT, PpsJobPrintClass))

typedef enum {
	PPS_JOB_RUN_THREAD,
	PPS_JOB_RUN_MAIN_LOOP
} PpsJobRunMode;

struct _PpsJob
{
	GObject parent;

	PpsDocument *document;

	PpsJobRunMode run_mode;

	guint cancelled : 1;
	guint finished : 1;
	guint failed : 1;

	GError *error;
	GCancellable *cancellable;

	guint idle_finished_id;
	guint idle_cancelled_id;
};

struct _PpsJobClass
{
	GObjectClass parent_class;

	gboolean (*run)         (PpsJob *job);

	/* Signals */
	void     (* cancelled)  (PpsJob *job);
	void     (* finished)   (PpsJob *job);
};

struct _PpsJobLinks
{
	PpsJob parent;

	GtkTreeModel *model;
};

struct _PpsJobLinksClass
{
	PpsJobClass parent_class;
};

struct _PpsJobAttachments
{
	PpsJob parent;

	GList *attachments;
};

struct _PpsJobAttachmentsClass
{
	PpsJobClass parent_class;
};

struct _PpsJobAnnots
{
	PpsJob parent;

	GList *annots;
};

struct _PpsJobAnnotsClass
{
	PpsJobClass parent_class;
};

struct _PpsJobRenderCairo
{
	PpsJob parent;

	gint page;
	gint rotation;
	gdouble scale;

	gboolean page_ready;
	gint target_width;
	gint target_height;
	cairo_surface_t *surface;

	gboolean include_selection;
	cairo_surface_t *selection;
	cairo_region_t *selection_region;
	PpsRectangle selection_points;
	PpsSelectionStyle selection_style;
	GdkRGBA base;
	GdkRGBA text;
};

struct _PpsJobRenderCairoClass
{
	PpsJobClass parent_class;
};

struct _PpsJobRenderTexture
{
	PpsJob parent;

	gint page;
	gint rotation;
	gdouble scale;

	gboolean page_ready;
	gint target_width;
	gint target_height;
	GdkTexture *texture;

	gboolean include_selection;
	GdkTexture *selection;
	cairo_region_t *selection_region;
	PpsRectangle selection_points;
	PpsSelectionStyle selection_style;
	GdkRGBA base;
	GdkRGBA text;
};

struct _PpsJobRenderTextureClass
{
	PpsJobClass parent_class;
};

typedef enum {
        PPS_PAGE_DATA_INCLUDE_NONE           = 0,
        PPS_PAGE_DATA_INCLUDE_LINKS          = 1 << 0,
        PPS_PAGE_DATA_INCLUDE_TEXT           = 1 << 1,
        PPS_PAGE_DATA_INCLUDE_TEXT_MAPPING   = 1 << 2,
        PPS_PAGE_DATA_INCLUDE_TEXT_LAYOUT    = 1 << 3,
        PPS_PAGE_DATA_INCLUDE_TEXT_ATTRS     = 1 << 4,
        PPS_PAGE_DATA_INCLUDE_TEXT_LOG_ATTRS = 1 << 5,
        PPS_PAGE_DATA_INCLUDE_IMAGES         = 1 << 6,
        PPS_PAGE_DATA_INCLUDE_FORMS          = 1 << 7,
        PPS_PAGE_DATA_INCLUDE_ANNOTS         = 1 << 8,
        PPS_PAGE_DATA_INCLUDE_MEDIA          = 1 << 9,
        PPS_PAGE_DATA_INCLUDE_ALL            = (1 << 10) - 1
} PpsJobPageDataFlags;

struct _PpsJobPageData
{
	PpsJob parent;

	gint page;
	PpsJobPageDataFlags flags;

	PpsMappingList  *link_mapping;
	PpsMappingList  *image_mapping;
	PpsMappingList  *form_field_mapping;
	PpsMappingList  *annot_mapping;
        PpsMappingList  *media_mapping;
	cairo_region_t *text_mapping;
	gchar *text;
	PpsRectangle *text_layout;
	guint text_layout_length;
        PangoAttrList *text_attrs;
        PangoLogAttr *text_log_attrs;
        gulong text_log_attrs_length;
};

struct _PpsJobPageDataClass
{
	PpsJobClass parent_class;
};

struct _PpsJobThumbnailCairo
{
	PpsJob parent;

	gint page;
	gint rotation;
	gdouble scale;
	gint target_width;
	gint target_height;

        cairo_surface_t *thumbnail_surface;
};

struct _PpsJobThumbnailCairoClass
{
	PpsJobClass parent_class;
};

struct _PpsJobThumbnailTexture
{
	PpsJob parent;

	gint page;
	gint rotation;
	gdouble scale;
	gint target_width;
	gint target_height;

        GdkTexture *thumbnail_texture;
};

struct _PpsJobThumbnailTextureClass
{
	PpsJobClass parent_class;
};

struct _PpsJobFonts
{
	PpsJob parent;
};

struct _PpsJobFontsClass
{
	PpsJobClass parent_class;
};

struct _PpsJobLoad
{
	PpsJob parent;

	gchar *uri;
	int fd;
	char *mime_type;
	gchar *password;
	PpsDocumentLoadFlags flags;
	PpsDocument *loaded_document;
};

struct _PpsJobLoadClass
{
	PpsJobClass parent_class;
};

struct _PpsJobSave
{
	PpsJob parent;

	gchar *uri;
	gchar *document_uri;
};

struct _PpsJobSaveClass
{
	PpsJobClass parent_class;
};

struct _PpsJobFind
{
	PpsJob parent;

	gint start_page;
	gint current_page;
	gint n_pages;
	GList **pages;
	gchar *text;
	gboolean has_results;
        PpsFindOptions options;
};

struct _PpsJobFindClass
{
	PpsJobClass parent_class;

	/* Signals */
	void (* updated)  (PpsJobFind *job,
			   gint       page);
};

struct _PpsJobLayers
{
	PpsJob parent;

	GtkTreeModel *model;
};

struct _PpsJobLayersClass
{
	PpsJobClass parent_class;
};

struct _PpsJobExport
{
	PpsJob parent;

	gint page;
	PpsRenderContext *rc;
};

struct _PpsJobExportClass
{
	PpsJobClass parent_class;
};

struct _PpsJobPrint
{
	PpsJob parent;

	gint page;
	cairo_t *cr;
};

struct _PpsJobPrintClass
{
	PpsJobClass parent_class;
};

/* Base job class */
PPS_PUBLIC
GType           pps_job_get_type           (void) G_GNUC_CONST;
PPS_PUBLIC
gboolean        pps_job_run                (PpsJob          *job);
PPS_PUBLIC
void            pps_job_cancel             (PpsJob          *job);
PPS_PUBLIC
void            pps_job_failed             (PpsJob          *job,
					   GQuark          domain,
					   gint            code,
					   const gchar    *format,
					   ...) G_GNUC_PRINTF (4, 5);
PPS_PUBLIC
void            pps_job_failed_from_error  (PpsJob          *job,
					   GError         *error);
PPS_PUBLIC
void            pps_job_succeeded          (PpsJob          *job);
PPS_PUBLIC
gboolean        pps_job_is_finished        (PpsJob          *job);
PPS_PUBLIC
gboolean        pps_job_is_failed          (PpsJob          *job);
PPS_PUBLIC
PpsJobRunMode    pps_job_get_run_mode       (PpsJob          *job);
PPS_PUBLIC
void            pps_job_set_run_mode       (PpsJob          *job,
					   PpsJobRunMode    run_mode);
PPS_PUBLIC
PpsDocument     *pps_job_get_document	  (PpsJob	  *job);

/* PpsJobLinks */
PPS_PUBLIC
GType           pps_job_links_get_type     (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_links_new          (PpsDocument     *document);
PPS_PUBLIC
GtkTreeModel   *pps_job_links_get_model    (PpsJobLinks     *job);

/* PpsJobAttachments */
PPS_PUBLIC
GType           pps_job_attachments_get_type (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_attachments_new      (PpsDocument     *document);

/* PpsJobAnnots */
PPS_PUBLIC
GType           pps_job_annots_get_type      (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_annots_new           (PpsDocument     *document);

/* PpsJobRenderTexture */
PPS_PUBLIC
GType           pps_job_render_texture_get_type    (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_render_texture_new         (PpsDocument      *document,
						 gint             page,
						 gint             rotation,
						 gdouble          scale,
						 gint             width,
						 gint             height);
PPS_PUBLIC
void     pps_job_render_texture_set_selection_info (PpsJobRenderTexture     *job,
						 PpsRectangle     *selection_points,
						 PpsSelectionStyle selection_style,
						 GdkRGBA         *text,
						 GdkRGBA         *base);

/* PpsJobPageData */
PPS_PUBLIC
GType           pps_job_page_data_get_type (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_page_data_new      (PpsDocument      *document,
					   gint             page,
					   PpsJobPageDataFlags flags);

/* PpsJobThumbnailCairo */
PPS_DEPRECATED PPS_PUBLIC
GType           pps_job_thumbnail_cairo_get_type      (void) G_GNUC_CONST;
PPS_DEPRECATED PPS_PUBLIC
PpsJob          *pps_job_thumbnail_cairo_new           (PpsDocument      *document,
						      gint             page,
						      gint             rotation,
						      gdouble          scale);
PPS_DEPRECATED PPS_PUBLIC
PpsJob          *pps_job_thumbnail_cairo_new_with_target_size (PpsDocument *document,
							     gint        page,
							     gint        rotation,
							     gint        target_width,
							     gint        target_height);

/* PpsJobThumbnailTexture */
PPS_PUBLIC
GType           pps_job_thumbnail_texture_get_type      (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_thumbnail_texture_new           (PpsDocument      *document,
						      gint             page,
						      gint             rotation,
						      gdouble          scale);
PPS_PUBLIC
PpsJob          *pps_job_thumbnail_texture_new_with_target_size (PpsDocument *document,
							     gint        page,
							     gint        rotation,
							     gint        target_width,
							     gint        target_height);
PPS_PUBLIC
GdkTexture     *pps_job_thumbnail_texture_get_texture (PpsJobThumbnailTexture *job);

/* PpsJobFonts */
PPS_PUBLIC
GType 		pps_job_fonts_get_type 	  (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob 	       *pps_job_fonts_new 	  (PpsDocument      *document);

/* PpsJobLoad */
PPS_PUBLIC
GType           pps_job_load_get_type      (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_load_new           (void);
PPS_PUBLIC
void            pps_job_load_set_uri       (PpsJobLoad       *job,
					   const gchar     *uri);
PPS_PUBLIC
gboolean        pps_job_load_set_fd        (PpsJobLoad       *job,
					   int              fd,
					   const char      *mime_type,
					   GError         **error);
PPS_PUBLIC
void            pps_job_load_take_fd       (PpsJobLoad       *job,
					   int              fd,
					   const char      *mime_type);
PPS_PUBLIC
void            pps_job_load_set_password  (PpsJobLoad       *job,
					   const gchar     *password);
PPS_PUBLIC
void           pps_job_load_set_load_flags (PpsJobLoad           *job,
					   PpsDocumentLoadFlags  flags);
PPS_PUBLIC
PpsDocument     *pps_job_load_get_loaded_document (PpsJobLoad *job);

/* PpsJobSave */
PPS_PUBLIC
GType           pps_job_save_get_type      (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_save_new           (PpsDocument      *document,
					   const gchar     *uri,
					   const gchar     *document_uri);
/* PpsJobFind */
PPS_PUBLIC
GType           pps_job_find_get_type      (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_find_new           (PpsDocument      *document,
					   gint             start_page,
					   gint             n_pages,
					   const gchar     *text,
					   PpsFindOptions    options);
PPS_PUBLIC
PpsFindOptions   pps_job_find_get_options   (PpsJobFind       *job);
PPS_PUBLIC
gint            pps_job_find_get_n_main_results (PpsJobFind  *job,
						gint        page);
PPS_PUBLIC
gdouble         pps_job_find_get_progress  (PpsJobFind       *job);
PPS_PUBLIC
gboolean        pps_job_find_has_results   (PpsJobFind       *job);
PPS_PUBLIC
GList         **pps_job_find_get_results   (PpsJobFind       *job);

/* PpsJobLayers */
PPS_PUBLIC
GType           pps_job_layers_get_type    (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_layers_new         (PpsDocument     *document);

/* PpsJobExport */
PPS_PUBLIC
GType           pps_job_export_get_type    (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_export_new         (PpsDocument     *document);
PPS_PUBLIC
void            pps_job_export_set_page    (PpsJobExport    *job,
					   gint            page);
/* PpsJobPrint */
PPS_PUBLIC
GType           pps_job_print_get_type    (void) G_GNUC_CONST;
PPS_PUBLIC
PpsJob          *pps_job_print_new         (PpsDocument     *document);
PPS_PUBLIC
void            pps_job_print_set_page    (PpsJobPrint     *job,
					  gint            page);
PPS_PUBLIC
void            pps_job_print_set_cairo   (PpsJobPrint     *job,
					  cairo_t        *cr);

G_END_DECLS
