// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2010 Yaco Sistemas, Daniel Garcia <danigm@yaco.es>
 */

#include "config.h"

#include "pps-document-text.h"

G_DEFINE_INTERFACE (PpsDocumentText, pps_document_text, 0)

static void
pps_document_text_default_init (PpsDocumentTextInterface *klass)
{
}

gchar *
pps_document_text_get_text (PpsDocumentText *document_text,
                            PpsPage *page)
{
	PpsDocumentTextInterface *iface = PPS_DOCUMENT_TEXT_GET_IFACE (document_text);

	if (!iface->get_text)
		return NULL;

	return iface->get_text (document_text, page);
}

/**
 * pps_document_text_get_text_layout:
 * @document_text: a #PpsDocumentText
 * @page: a #PpsPage
 * @areas: (optional) (out) (array length=n_areas) (transfer container): the text areas of the layout
 * @n_areas: (out): the number of text areas in the layout
 *
 * Returns: whether the text layout is empty, i.e. there is text on the page
 */
gboolean
pps_document_text_get_text_layout (PpsDocumentText *document_text,
                                   PpsPage *page,
                                   PpsRectangle **areas,
                                   guint *n_areas)
{
	PpsDocumentTextInterface *iface = PPS_DOCUMENT_TEXT_GET_IFACE (document_text);

	if (!iface->get_text_layout)
		return FALSE;

	return iface->get_text_layout (document_text, page, areas, n_areas);
}

/**
 * pps_document_text_get_text_in_area:
 * @document_text: a #PpsDocumentText
 * @page: a #PpsPage of that document
 * @area: a #PpsRectangle on that page
 *
 * Returns: (transfer full) (nullable): the text inside the area of the specified page or %NULL
 *
 * Since: 47
 */
gchar *
pps_document_text_get_text_in_area (PpsDocumentText *document_text,
                                    PpsPage *page,
                                    PpsRectangle *area)
{
	PpsDocumentTextInterface *iface = PPS_DOCUMENT_TEXT_GET_IFACE (document_text);

	if (!iface->get_text_in_area)
		return NULL;

	return iface->get_text_in_area (document_text, page, area);
}

cairo_region_t *
pps_document_text_get_text_mapping (PpsDocumentText *document_text,
                                    PpsPage *page)
{
	PpsDocumentTextInterface *iface = PPS_DOCUMENT_TEXT_GET_IFACE (document_text);

	if (!iface->get_text_mapping)
		return NULL;

	return iface->get_text_mapping (document_text, page);
}

/**
 * pps_document_text_get_text_attrs:
 * @document_text: a #PpsDocumentText
 * @page: a #PpsPage
 *
 * FIXME
 *
 * Returns: (transfer full): a newly created #PangoAttrList
 */
PangoAttrList *
pps_document_text_get_text_attrs (PpsDocumentText *document_text,
                                  PpsPage *page)
{
	PpsDocumentTextInterface *iface = PPS_DOCUMENT_TEXT_GET_IFACE (document_text);

	if (!iface->get_text_attrs)
		return NULL;

	return iface->get_text_attrs (document_text, page);
}
