// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-print.c
 *  this file is part of papers, a gnome document_links viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos  <carlosgc@gnome.org>
 */

#include "config.h"

#include "pps-document-print.h"
#include "pps-document.h"

G_DEFINE_INTERFACE (PpsDocumentPrint, pps_document_print, 0)

static void
pps_document_print_default_init (PpsDocumentPrintInterface *klass)
{
}

void
pps_document_print_print_page (PpsDocumentPrint *document_print,
                               PpsPage *page,
                               cairo_t *cr)
{
	PpsDocumentPrintInterface *iface = PPS_DOCUMENT_PRINT_GET_IFACE (document_print);

	iface->print_page (document_print, page, cr);
}
