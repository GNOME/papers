// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-images.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2006 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>
#include <glib.h>

#include "pps-document.h"
#include "pps-image.h"
#include "pps-macros.h"
#include "pps-mapping-list.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_IMAGES (pps_document_images_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentImages, pps_document_images, PPS, DOCUMENT_IMAGES, GObject)

struct _PpsDocumentImagesInterface {
	GTypeInterface base_iface;

	/* Methods  */
	PpsMappingList *(*get_image_mapping) (PpsDocumentImages *document_images,
	                                      PpsPage *page);
	GdkPixbuf *(*get_image) (PpsDocumentImages *document_images,
	                         PpsImage *image);
};

PPS_PUBLIC
PpsMappingList *pps_document_images_get_image_mapping (PpsDocumentImages *document_images,
                                                       PpsPage *page);
PPS_PUBLIC
GdkPixbuf *pps_document_images_get_image (PpsDocumentImages *document_images,
                                          PpsImage *image);

G_END_DECLS
