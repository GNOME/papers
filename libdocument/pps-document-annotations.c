// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-annotations.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2007 Iñigo Martinez <inigomartinez@gmail.com>
 */

#include "pps-document-annotations.h"

G_DEFINE_INTERFACE (PpsDocumentAnnotations, pps_document_annotations, 0)

static void
pps_document_annotations_default_init (PpsDocumentAnnotationsInterface *klass)
{
}

/**
 * pps_document_annotations_get_annotations:
 * @document_annots: a #PpsDocumentAnnotations
 * @page: the page from where to get the annotations
 *
 * Returns: (nullable) (transfer none) (element-type PpsAnnotation):
 */
GList *
pps_document_annotations_get_annotations (PpsDocumentAnnotations *document_annots,
                                          PpsPage *page)
{
	PpsDocumentAnnotationsInterface *iface = PPS_DOCUMENT_ANNOTATIONS_GET_IFACE (document_annots);

	return iface->get_annotations (document_annots, page);
}

gboolean
pps_document_annotations_document_is_modified (PpsDocumentAnnotations *document_annots)
{
	PpsDocumentAnnotationsInterface *iface = PPS_DOCUMENT_ANNOTATIONS_GET_IFACE (document_annots);

	return (iface->document_is_modified) ? iface->document_is_modified (document_annots) : FALSE;
}

void
pps_document_annotations_add_annotation (PpsDocumentAnnotations *document_annots,
                                         PpsAnnotation *annot)
{
	PpsDocumentAnnotationsInterface *iface = PPS_DOCUMENT_ANNOTATIONS_GET_IFACE (document_annots);

	if (iface->add_annotation)
		iface->add_annotation (document_annots, annot);
}

gboolean
pps_document_annotations_can_add_annotation (PpsDocumentAnnotations *document_annots)
{
	PpsDocumentAnnotationsInterface *iface = PPS_DOCUMENT_ANNOTATIONS_GET_IFACE (document_annots);

	return iface->add_annotation != NULL;
}

void
pps_document_annotations_remove_annotation (PpsDocumentAnnotations *document_annots,
                                            PpsAnnotation *annot)
{
	PpsDocumentAnnotationsInterface *iface = PPS_DOCUMENT_ANNOTATIONS_GET_IFACE (document_annots);

	if (iface->remove_annotation)
		iface->remove_annotation (document_annots, annot);
}

gboolean
pps_document_annotations_can_remove_annotation (PpsDocumentAnnotations *document_annots)
{
	PpsDocumentAnnotationsInterface *iface = PPS_DOCUMENT_ANNOTATIONS_GET_IFACE (document_annots);

	return iface->remove_annotation != NULL;
}

PpsAnnotationsOverMarkup
pps_document_annotations_over_markup (PpsDocumentAnnotations *document_annots,
                                      PpsAnnotation *annot,
                                      gdouble x,
                                      gdouble y)
{
	PpsDocumentAnnotationsInterface *iface = PPS_DOCUMENT_ANNOTATIONS_GET_IFACE (document_annots);

	if (iface->over_markup)
		return iface->over_markup (document_annots, annot, x, y);

	return PPS_ANNOTATION_OVER_MARKUP_NOT_IMPLEMENTED;
}
