// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-attachments.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos  <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>
#include <glib.h>

#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_ATTACHMENTS (pps_document_attachments_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentAttachments, pps_document_attachments, PPS, DOCUMENT_ATTACHMENTS, GObject)

struct _PpsDocumentAttachmentsInterface {
	GTypeInterface base_iface;

	/* Methods  */
	gboolean (*has_attachments) (PpsDocumentAttachments *document_attachments);
	GList *(*get_attachments) (PpsDocumentAttachments *document_attachments);
};

PPS_PUBLIC
gboolean pps_document_attachments_has_attachments (PpsDocumentAttachments *document_attachments);
PPS_PUBLIC
GList *pps_document_attachments_get_attachments (PpsDocumentAttachments *document_attachments);

G_END_DECLS
