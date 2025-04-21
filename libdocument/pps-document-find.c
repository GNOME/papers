// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2004 Red Hat, Inc.
 */

#include "config.h"

#include "pps-document-find.h"

G_DEFINE_INTERFACE (PpsDocumentFind, pps_document_find, 0)

static void
pps_document_find_default_init (PpsDocumentFindInterface *klass)
{
}

/**
 * pps_document_find_find_text:
 * @document_find: an #PpsDocumentFind
 * @page: an #PpsPage
 * @text: text to find
 * @options: a set of #PpsFindOptions
 *
 * Returns: (transfer full) (element-type PpsFindRectangle): a list of results
 */
GList *
pps_document_find_find_text (PpsDocumentFind *document_find,
                             PpsPage *page,
                             const gchar *text,
                             PpsFindOptions options)
{
	PpsDocumentFindInterface *iface = PPS_DOCUMENT_FIND_GET_IFACE (document_find);

	return iface->find_text (document_find, page, text, options);
}

/* PpsFindRectangle */
G_DEFINE_BOXED_TYPE (PpsFindRectangle, pps_find_rectangle, pps_find_rectangle_copy, pps_find_rectangle_free)

PpsFindRectangle *
pps_find_rectangle_new (void)
{
	return g_slice_new0 (PpsFindRectangle);
}

PpsFindRectangle *
pps_find_rectangle_copy (PpsFindRectangle *rectangle)
{
	g_return_val_if_fail (rectangle != NULL, NULL);
	return g_slice_dup (PpsFindRectangle, rectangle);
}

void
pps_find_rectangle_free (PpsFindRectangle *rectangle)
{
	g_slice_free (PpsFindRectangle, rectangle);
}

PpsFindOptions
pps_document_find_get_supported_options (PpsDocumentFind *document_find)
{
	PpsDocumentFindInterface *iface = PPS_DOCUMENT_FIND_GET_IFACE (document_find);

	if (iface->get_supported_options)
		return iface->get_supported_options (document_find);
	return 0;
}
