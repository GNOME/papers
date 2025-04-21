// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-media.c
 *  this file is part of papers, a gnome document_links viewer
 *
 * Copyright (C) 2015 Igalia S.L.
 */

#include "pps-document-media.h"
#include <config.h>

G_DEFINE_INTERFACE (PpsDocumentMedia, pps_document_media, 0)

static void
pps_document_media_default_init (PpsDocumentMediaInterface *klass)
{
}

PpsMappingList *
pps_document_media_get_media_mapping (PpsDocumentMedia *document_media,
                                      PpsPage *page)
{
	PpsDocumentMediaInterface *iface = PPS_DOCUMENT_MEDIA_GET_IFACE (document_media);

	return iface->get_media_mapping (document_media, page);
}
