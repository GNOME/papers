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
gboolean pps_document_annotations_can_add_annotation (PpsDocumentAnnotations *document_annots);
PPS_PUBLIC
gboolean pps_document_annotations_can_remove_annotation (PpsDocumentAnnotations *document_annots);
PPS_PUBLIC
PpsAnnotationsOverMarkup pps_document_annotations_over_markup (PpsDocumentAnnotations *document_annots,
                                                               PpsAnnotation *annot,
                                                               gdouble x,
                                                               gdouble y);

G_END_DECLS
