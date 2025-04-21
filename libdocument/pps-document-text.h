// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2010 Yaco Sistemas, Daniel Garcia <danigm@yaco.es>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>
#include <glib.h>
#include <pango/pango.h>

#include "pps-document.h"
#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_TEXT (pps_document_text_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentText, pps_document_text, PPS, DOCUMENT_TEXT, GObject)

struct _PpsDocumentTextInterface {
	GTypeInterface base_iface;

	/* Methods */
	cairo_region_t *(*get_text_mapping) (PpsDocumentText *document_text,
	                                     PpsPage *page);
	gchar *(*get_text) (PpsDocumentText *document_text,
	                    PpsPage *page);
	gboolean (*get_text_layout) (PpsDocumentText *document_text,
	                             PpsPage *page,
	                             PpsRectangle **areas,
	                             guint *n_areas);
	gchar *(*get_text_in_area) (PpsDocumentText *document_text,
	                            PpsPage *page,
	                            PpsRectangle *area);
	PangoAttrList *(*get_text_attrs) (PpsDocumentText *document_text,
	                                  PpsPage *page);
};

PPS_PUBLIC
gchar *pps_document_text_get_text (PpsDocumentText *document_text,
                                   PpsPage *page);
PPS_PUBLIC
gboolean pps_document_text_get_text_layout (PpsDocumentText *document_text,
                                            PpsPage *page,
                                            PpsRectangle **areas,
                                            guint *n_areas);
PPS_PUBLIC
gchar *pps_document_text_get_text_in_area (PpsDocumentText *document_text,
                                           PpsPage *page,
                                           PpsRectangle *area);
PPS_PUBLIC
cairo_region_t *pps_document_text_get_text_mapping (PpsDocumentText *document_text,
                                                    PpsPage *page);
PPS_PUBLIC
PangoAttrList *pps_document_text_get_text_attrs (PpsDocumentText *document_text,
                                                 PpsPage *page);
G_END_DECLS
