// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-annotation.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2007 Iñigo Martinez <inigomartinez@gmail.com>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <glib-object.h>

#include "pps-attachment.h"
#include "pps-document.h"
#include "pps-macros.h"

G_BEGIN_DECLS

/* PpsAnnotation */
#define PPS_TYPE_ANNOTATION (pps_annotation_get_type ())
PPS_PUBLIC
G_DECLARE_DERIVABLE_TYPE (PpsAnnotation, pps_annotation, PPS, ANNOTATION, GObject);

struct _PpsAnnotationClass {
	GObjectClass parent_class;
};

/* PpsAnnotationMarkup */
#define PPS_TYPE_ANNOTATION_MARKUP (pps_annotation_markup_get_type ())

PPS_PUBLIC
G_DECLARE_DERIVABLE_TYPE (PpsAnnotationMarkup, pps_annotation_markup, PPS, ANNOTATION_MARKUP, PpsAnnotation);

struct _PpsAnnotationMarkupClass {
	PpsAnnotationClass parent_class;
};

/* PpsAnnotationText */
#define PPS_TYPE_ANNOTATION_TEXT (pps_annotation_text_get_type ())
PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsAnnotationText, pps_annotation_text, PPS, ANNOTATION_TEXT, PpsAnnotation);

/* PpsAnnotationStamp */
#define PPS_TYPE_ANNOTATION_STAMP (pps_annotation_stamp_get_type ())
PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsAnnotationStamp, pps_annotation_stamp, PPS, ANNOTATION_STAMP, PpsAnnotation);

/* PpsAnnotationFreeText */
#define PPS_TYPE_ANNOTATION_FREE_TEXT (pps_annotation_free_text_get_type ())
PPS_PUBLIC
G_DECLARE_FINAL_TYPE (
    PpsAnnotationFreeText, pps_annotation_free_text, PPS, ANNOTATION_FREE_TEXT, PpsAnnotation);

/* PpsAnnotationAttachment */
#define PPS_TYPE_ANNOTATION_ATTACHMENT (pps_annotation_attachment_get_type ())
PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsAnnotationAttachment, pps_annotation_attachment, PPS, ANNOTATION_ATTACHMENT, PpsAnnotation);

/* PpsAnnotationTextMarkup */
#define PPS_TYPE_ANNOTATION_TEXT_MARKUP (pps_annotation_text_markup_get_type ())
PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsAnnotationTextMarkup, pps_annotation_text_markup, PPS, ANNOTATION_TEXT_MARKUP, PpsAnnotation);

typedef enum {
	PPS_ANNOTATION_TYPE_UNKNOWN,
	PPS_ANNOTATION_TYPE_TEXT,
	PPS_ANNOTATION_TYPE_FREE_TEXT,
	PPS_ANNOTATION_TYPE_ATTACHMENT,
	PPS_ANNOTATION_TYPE_TEXT_MARKUP,
	PPS_ANNOTATION_TYPE_STAMP
} PpsAnnotationType;

typedef enum {
	PPS_ANNOTATION_TEXT_ICON_NOTE,
	PPS_ANNOTATION_TEXT_ICON_COMMENT,
	PPS_ANNOTATION_TEXT_ICON_KEY,
	PPS_ANNOTATION_TEXT_ICON_HELP,
	PPS_ANNOTATION_TEXT_ICON_NEW_PARAGRAPH,
	PPS_ANNOTATION_TEXT_ICON_PARAGRAPH,
	PPS_ANNOTATION_TEXT_ICON_INSERT,
	PPS_ANNOTATION_TEXT_ICON_CROSS,
	PPS_ANNOTATION_TEXT_ICON_CIRCLE,
	PPS_ANNOTATION_TEXT_ICON_UNKNOWN
} PpsAnnotationTextIcon;

typedef enum {
	PPS_ANNOTATION_TEXT_MARKUP_HIGHLIGHT,
	PPS_ANNOTATION_TEXT_MARKUP_STRIKE_OUT,
	PPS_ANNOTATION_TEXT_MARKUP_UNDERLINE,
	PPS_ANNOTATION_TEXT_MARKUP_SQUIGGLY
} PpsAnnotationTextMarkupType;

/* yellow-4 (#f5c211) from libadwaita */
#define PPS_ANNOTATION_DEFAULT_COLOR ((const GdkRGBA) { 0.960784, 0.760784, 0.066666, 1. });

/* PpsAnnotation */
PPS_PUBLIC
PpsAnnotationType pps_annotation_get_annotation_type (PpsAnnotation *annot);
PPS_PUBLIC
PpsPage *pps_annotation_get_page (PpsAnnotation *annot);
PPS_PUBLIC
guint pps_annotation_get_page_index (PpsAnnotation *annot);
PPS_PUBLIC
gboolean pps_annotation_equal (PpsAnnotation *annot,
                               PpsAnnotation *other);
PPS_PUBLIC
const gchar *pps_annotation_get_contents (PpsAnnotation *annot);
PPS_PUBLIC
gboolean pps_annotation_set_contents (PpsAnnotation *annot,
                                      const gchar *contents);
PPS_PUBLIC
const gchar *pps_annotation_get_name (PpsAnnotation *annot);
PPS_PUBLIC
gboolean pps_annotation_set_name (PpsAnnotation *annot,
                                  const gchar *name);
PPS_PUBLIC
const gchar *pps_annotation_get_modified (PpsAnnotation *annot);
PPS_PUBLIC
gboolean pps_annotation_set_modified (PpsAnnotation *annot,
                                      const gchar *modified);
PPS_PUBLIC
gboolean pps_annotation_set_hidden (PpsAnnotation *annot,
                                    const gboolean hidden);
PPS_PUBLIC
gboolean pps_annotation_get_hidden (PpsAnnotation *annot);
PPS_PUBLIC
gboolean pps_annotation_set_border_width (PpsAnnotation *annot,
                                          const gdouble width);
PPS_PUBLIC
gdouble pps_annotation_get_border_width (PpsAnnotation *annot);
PPS_PUBLIC
gboolean pps_annotation_set_modified_from_time_t (PpsAnnotation *annot,
                                                  time_t utime);
PPS_PUBLIC
void pps_annotation_get_rgba (PpsAnnotation *annot,
                              GdkRGBA *rgba);
PPS_PUBLIC
gboolean pps_annotation_set_rgba (PpsAnnotation *annot,
                                  const GdkRGBA *rgba);
PPS_PUBLIC
void pps_annotation_get_area (PpsAnnotation *annot,
                              PpsRectangle *area);
PPS_PUBLIC
gboolean pps_annotation_set_area (PpsAnnotation *annot,
                                  const PpsRectangle *area);

/* Undo */
PPS_PUBLIC
void pps_annotation_get_value_last_property (PpsAnnotation *annot, GValue *value);

/* PpsAnnotationMarkup */
PPS_PUBLIC
const gchar *pps_annotation_markup_get_label (PpsAnnotationMarkup *markup);
PPS_PUBLIC
gboolean pps_annotation_markup_set_label (PpsAnnotationMarkup *markup,
                                          const gchar *label);
PPS_PUBLIC
gdouble pps_annotation_markup_get_opacity (PpsAnnotationMarkup *markup);
PPS_PUBLIC
gboolean pps_annotation_markup_set_opacity (PpsAnnotationMarkup *markup,
                                            gdouble opacity);
PPS_PUBLIC
gboolean pps_annotation_markup_can_have_popup (PpsAnnotationMarkup *markup);
PPS_PUBLIC
gboolean pps_annotation_markup_has_popup (PpsAnnotationMarkup *markup);
PPS_PUBLIC
gboolean pps_annotation_markup_set_has_popup (PpsAnnotationMarkup *markup,
                                              gboolean has_popup);
PPS_PUBLIC
void pps_annotation_markup_get_rectangle (PpsAnnotationMarkup *markup,
                                          PpsRectangle *pps_rect);
PPS_PUBLIC
gboolean pps_annotation_markup_set_rectangle (PpsAnnotationMarkup *markup,
                                              const PpsRectangle *pps_rect);
PPS_PUBLIC
gboolean pps_annotation_markup_get_popup_is_open (PpsAnnotationMarkup *markup);
PPS_PUBLIC
gboolean pps_annotation_markup_set_popup_is_open (PpsAnnotationMarkup *markup,
                                                  gboolean is_open);

/* PpsAnnotationText */
PPS_PUBLIC
PpsAnnotation *pps_annotation_text_new (PpsPage *page);
PPS_PUBLIC
PpsAnnotationTextIcon pps_annotation_text_get_icon (PpsAnnotationText *text);
PPS_PUBLIC
gboolean pps_annotation_text_set_icon (PpsAnnotationText *text,
                                       PpsAnnotationTextIcon icon);
PPS_PUBLIC
gboolean pps_annotation_text_get_is_open (PpsAnnotationText *text);
PPS_PUBLIC
gboolean pps_annotation_text_set_is_open (PpsAnnotationText *text,
                                          gboolean is_open);

/* PpsAnnotationStamp */
PPS_PUBLIC
PpsAnnotation *pps_annotation_stamp_new (PpsPage *page);
PPS_PUBLIC
void pps_annotation_stamp_set_surface (PpsAnnotationStamp *stamp, cairo_surface_t *surface);
PPS_PUBLIC
cairo_surface_t *pps_annotation_stamp_get_surface (PpsAnnotationStamp *stamp);

/* PpsAnnotationFreeText */
PPS_PUBLIC
PpsAnnotation *pps_annotation_free_text_new (PpsPage *page);
PPS_PUBLIC
gboolean pps_annotation_free_text_set_font_description (PpsAnnotationFreeText *annot,
                                                        const PangoFontDescription *font_desc);
PPS_PUBLIC
PangoFontDescription *pps_annotation_free_text_get_font_description (PpsAnnotationFreeText *annot);
PPS_PUBLIC
gboolean pps_annotation_free_text_set_font_rgba (PpsAnnotationFreeText *annot,
                                                 const GdkRGBA *rgba);
PPS_PUBLIC
GdkRGBA *pps_annotation_free_text_get_font_rgba (PpsAnnotationFreeText *annot);
PPS_PUBLIC
void pps_annotation_free_text_auto_resize (PpsAnnotationFreeText *annot,
                                           PangoContext *ctx);

/* PpsAnnotationAttachment */
PPS_PUBLIC
PpsAnnotation *pps_annotation_attachment_new (PpsPage *page,
                                              PpsAttachment *attachment);
PPS_PUBLIC
PpsAttachment *pps_annotation_attachment_get_attachment (PpsAnnotationAttachment *annot);
PPS_PUBLIC
gboolean pps_annotation_attachment_set_attachment (PpsAnnotationAttachment *annot,
                                                   PpsAttachment *attachment);

/* PpsAnnotationTextMarkup */
PPS_PUBLIC
PpsAnnotation *pps_annotation_text_markup_new (PpsPage *page, PpsAnnotationTextMarkupType markup_type);
PPS_PUBLIC
PpsAnnotation *pps_annotation_text_markup_highlight_new (PpsPage *page);
PPS_PUBLIC
PpsAnnotation *pps_annotation_text_markup_strike_out_new (PpsPage *page);
PPS_PUBLIC
PpsAnnotation *pps_annotation_text_markup_underline_new (PpsPage *page);
PPS_PUBLIC
PpsAnnotation *pps_annotation_text_markup_squiggly_new (PpsPage *page);
PPS_PUBLIC
PpsAnnotationTextMarkupType pps_annotation_text_markup_get_markup_type (PpsAnnotationTextMarkup *annot);
PPS_PUBLIC
gboolean pps_annotation_text_markup_set_markup_type (PpsAnnotationTextMarkup *annot,
                                                     PpsAnnotationTextMarkupType markup_type);

G_END_DECLS
