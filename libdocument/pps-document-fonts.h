// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-fonts.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2004 Red Hat, Inc.
 *
 * Author:
 *   Marco Pesenti Gritti <mpg@redhat.com>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>
#include <glib.h>

#include "pps-document.h"
#include "pps-link.h"
#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_FONTS (pps_document_fonts_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentFonts, pps_document_fonts, PPS, DOCUMENT_FONTS, GObject)

enum {
	PPS_DOCUMENT_FONTS_COLUMN_NAME,
	PPS_DOCUMENT_FONTS_COLUMN_DETAILS,
	PPS_DOCUMENT_FONTS_COLUMN_NUM_COLUMNS
};

struct _PpsDocumentFontsInterface {
	GTypeInterface base_iface;

	/* Methods */
	void (*scan) (PpsDocumentFonts *document_fonts);
	GListModel *(*get_model) (PpsDocumentFonts *document_fonts);
	const gchar *(*get_fonts_summary) (PpsDocumentFonts *document_fonts);
};

PPS_PUBLIC
void pps_document_fonts_scan (PpsDocumentFonts *document_fonts);
PPS_PUBLIC
GListModel *pps_document_fonts_get_model (PpsDocumentFonts *document_fonts);
PPS_PUBLIC
const gchar *pps_document_fonts_get_fonts_summary (PpsDocumentFonts *document_fonts);

G_END_DECLS
