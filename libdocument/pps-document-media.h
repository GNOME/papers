// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-media.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2015 Igalia S.L.
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>
#include <glib.h>

#include "pps-document.h"
#include "pps-macros.h"
#include "pps-mapping-list.h"
#include "pps-media.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_MEDIA (pps_document_media_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentMedia, pps_document_media, PPS, DOCUMENT_MEDIA, GObject)

struct _PpsDocumentMediaInterface {
	GTypeInterface base_iface;

	/* Methods  */
	PpsMappingList *(*get_media_mapping) (PpsDocumentMedia *document_media,
	                                      PpsPage *page);
};

PPS_PUBLIC
PpsMappingList *pps_document_media_get_media_mapping (PpsDocumentMedia *document_media,
                                                      PpsPage *page);

G_END_DECLS
