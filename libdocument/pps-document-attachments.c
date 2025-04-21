// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-attachments.c
 *  this file is part of papers, a gnome document_links viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos  <carlosgc@gnome.org>
 */

#include "config.h"

#include "pps-document-attachments.h"
#include "pps-document.h"

G_DEFINE_INTERFACE (PpsDocumentAttachments, pps_document_attachments, 0)

static void
pps_document_attachments_default_init (PpsDocumentAttachmentsInterface *klass)
{
}

gboolean
pps_document_attachments_has_attachments (PpsDocumentAttachments *document_attachments)
{
	PpsDocumentAttachmentsInterface *iface = PPS_DOCUMENT_ATTACHMENTS_GET_IFACE (document_attachments);

	return iface->has_attachments (document_attachments);
}

/**
 * pps_document_attachments_get_attachments:
 * @document_attachments: an #PpsDocumentAttachments
 *
 * Returns: (transfer full) (element-type PpsAttachment): a list of #PpsAttachment objects
 */
GList *
pps_document_attachments_get_attachments (PpsDocumentAttachments *document_attachments)
{
	PpsDocumentAttachmentsInterface *iface = PPS_DOCUMENT_ATTACHMENTS_GET_IFACE (document_attachments);

	return iface->get_attachments (document_attachments);
}
