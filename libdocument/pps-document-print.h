// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-print.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos  <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <cairo.h>
#include <glib-object.h>

#include "pps-macros.h"
#include "pps-page.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_PRINT (pps_document_print_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentPrint, pps_document_print, PPS, DOCUMENT_PRINT, GObject)

struct _PpsDocumentPrintInterface {
	GTypeInterface base_iface;

	/* Methods  */
	void (*print_page) (PpsDocumentPrint *document_print,
	                    PpsPage *page,
	                    cairo_t *cr);
};

PPS_PUBLIC
void pps_document_print_print_page (PpsDocumentPrint *document_print,
                                    PpsPage *page,
                                    cairo_t *cr);

G_END_DECLS
