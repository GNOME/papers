// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-annotations.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2007 Iñigo Martinez <inigomartinez@gmail.com>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-annotation.h"
#include "pps-document.h"
#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_ANNOTATIONS (pps_document_annotations_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentAnnotations, pps_document_annotations, PPS, DOCUMENT_ANNOTATIONS, GObject)

typedef enum {
	PPS_ANNOTATIONS_SAVE_NONE = 0,
	PPS_ANNOTATIONS_SAVE_CONTENTS = 1 << 0,
	PPS_ANNOTATIONS_SAVE_COLOR = 1 << 1,
	PPS_ANNOTATIONS_SAVE_AREA = 1 << 2,
	PPS_ANNOTATIONS_SAVE_HIDDEN = 1 << 3,

	/* Markup Annotations */
	PPS_ANNOTATIONS_SAVE_LABEL = 1 << 4,
	PPS_ANNOTATIONS_SAVE_OPACITY = 1 << 5,
	PPS_ANNOTATIONS_SAVE_POPUP_RECT = 1 << 6,
	PPS_ANNOTATIONS_SAVE_POPUP_IS_OPEN = 1 << 7,

	/* Text Annotations */
	PPS_ANNOTATIONS_SAVE_TEXT_IS_OPEN = 1 << 8,
	PPS_ANNOTATIONS_SAVE_TEXT_ICON = 1 << 9,

	/* Attachment Annotations */
	PPS_ANNOTATIONS_SAVE_ATTACHMENT = 1 << 10,

	/* Text Markup Annotations */
	PPS_ANNOTATIONS_SAVE_TEXT_MARKUP_TYPE = 1 << 11,

	/* Free Text Annotations */
	PPS_ANNOTATIONS_SAVE_FREE_TEXT_FONT = 1 << 12,

	/* Save all */
	PPS_ANNOTATIONS_SAVE_ALL = (1 << 13) - 1
} PpsAnnotationsSaveMask;

typedef enum {
	PPS_ANNOTATION_OVER_MARKUP_NOT_IMPLEMENTED = 0,
	PPS_ANNOTATION_OVER_MARKUP_UNKNOWN,
	PPS_ANNOTATION_OVER_MARKUP_YES,
	PPS_ANNOTATION_OVER_MARKUP_NOT
} PpsAnnotationsOverMarkup;

struct _PpsDocumentAnnotationsInterface {
	GTypeInterface base_iface;

	/* Methods  */
	GList *(*get_annotations) (PpsDocumentAnnotations *document_annots,
	                           PpsPage *page);
	gboolean (*document_is_modified) (PpsDocumentAnnotations *document_annots);
	void (*add_annotation) (PpsDocumentAnnotations *document_annots,
	                        PpsAnnotation *annot);
	void (*save_annotation) (PpsDocumentAnnotations *document_annots,
	                         PpsAnnotation *annot,
	                         PpsAnnotationsSaveMask mask);
	void (*remove_annotation) (PpsDocumentAnnotations *document_annots,
	                           PpsAnnotation *annot);
	PpsAnnotationsOverMarkup (*over_markup) (PpsDocumentAnnotations *document_annots,
	                                         PpsAnnotation *annot,
	                                         gdouble x,
	                                         gdouble y);
};

PPS_PUBLIC
GList *pps_document_annotations_get_annotations (PpsDocumentAnnotations *document_annots,
                                                 PpsPage *page);
PPS_PUBLIC
gboolean pps_document_annotations_document_is_modified (PpsDocumentAnnotations *document_annots);
PPS_PUBLIC
void pps_document_annotations_add_annotation (PpsDocumentAnnotations *document_annots,
                                              PpsAnnotation *annot);
PPS_PUBLIC
void pps_document_annotations_remove_annotation (PpsDocumentAnnotations *document_annots,
                                                 PpsAnnotation *annot);

PPS_PUBLIC
void pps_document_annotations_save_annotation (PpsDocumentAnnotations *document_annots,
                                               PpsAnnotation *annot,
                                               PpsAnnotationsSaveMask mask);
PPS_PUBLIC
gboolean pps_document_annotations_can_add_annotation (PpsDocumentAnnotations *document_annots);
PPS_PUBLIC
gboolean pps_document_annotations_can_remove_annotation (PpsDocumentAnnotations *document_annots);
PPS_PUBLIC
PpsAnnotationsOverMarkup pps_document_annotations_over_markup (PpsDocumentAnnotations *document_annots,
                                                               PpsAnnotation *annot,
                                                               gdouble x,
                                                               gdouble y);

G_END_DECLS
