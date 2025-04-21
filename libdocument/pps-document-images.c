// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-images.c
 *  this file is part of papers, a gnome document_links viewer
 *
 * Copyright (C) 2006 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#include "pps-document-images.h"
#include <config.h>

G_DEFINE_INTERFACE (PpsDocumentImages, pps_document_images, 0)

static void
pps_document_images_default_init (PpsDocumentImagesInterface *klass)
{
}

PpsMappingList *
pps_document_images_get_image_mapping (PpsDocumentImages *document_images,
                                       PpsPage *page)
{
	PpsDocumentImagesInterface *iface = PPS_DOCUMENT_IMAGES_GET_IFACE (document_images);

	return iface->get_image_mapping (document_images, page);
}

/**
 * pps_document_images_get_image:
 * @document_images: an #PpsDocumentImages
 * @image: an #PpsImage
 *
 * Returns: (transfer full): a #GdkPixbuf
 */
GdkPixbuf *
pps_document_images_get_image (PpsDocumentImages *document_images,
                               PpsImage *image)
{
	PpsDocumentImagesInterface *iface = PPS_DOCUMENT_IMAGES_GET_IFACE (document_images);

	return iface->get_image (document_images, image);
}
