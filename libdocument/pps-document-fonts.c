// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-fonts.h
 *  this file is part of papers, a gnome document_fonts viewer
 *
 * Copyright (C) 2004 Red Hat, Inc.
 *
 * Author:
 *   Marco Pesenti Gritti <mpg@redhat.com>
 */

#include "config.h"

#include "pps-document-fonts.h"

G_DEFINE_INTERFACE (PpsDocumentFonts, pps_document_fonts, 0)

static void
pps_document_fonts_default_init (PpsDocumentFontsInterface *klass)
{
}

/**
 * pps_document_fonts_scan:
 * @document_fonts: a #PpsDocument which implements the #PpsDocumentFonts
 * interface
 *
 * Runs through the slow process of finding the fonts being used in a document.
 * To get the results of the scan, use pps_document_fonts_fill_model and
 * pps_document_fonts_get_fonts_summary
 */
void
pps_document_fonts_scan (PpsDocumentFonts *document_fonts)
{
	PpsDocumentFontsInterface *iface = PPS_DOCUMENT_FONTS_GET_IFACE (document_fonts);

	iface->scan (document_fonts);
}

/**
 * pps_document_fonts_get_model:
 * @document_fonts: a #PpsDocument which implements the #PpsDocumentFonts
 * interface
 *
 * Returns: (transfer full): A #GListModel holding #PpsFontDescription objects
 */
GListModel *
pps_document_fonts_get_model (PpsDocumentFonts *document_fonts)
{
	PpsDocumentFontsInterface *iface = PPS_DOCUMENT_FONTS_GET_IFACE (document_fonts);

	return iface->get_model (document_fonts);
}

const gchar *
pps_document_fonts_get_fonts_summary (PpsDocumentFonts *document_fonts)
{
	PpsDocumentFontsInterface *iface = PPS_DOCUMENT_FONTS_GET_IFACE (document_fonts);

	if (!iface->get_fonts_summary)
		return NULL;

	return iface->get_fonts_summary (document_fonts);
}
