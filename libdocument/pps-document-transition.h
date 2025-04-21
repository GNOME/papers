// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-transition.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2006 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>

#include "pps-document.h"
#include "pps-macros.h"
#include "pps-transition-effect.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_TRANSITION (pps_document_transition_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentTransition, pps_document_transition, PPS, DOCUMENT_TRANSITION, GObject)

struct _PpsDocumentTransitionInterface {
	GTypeInterface base_iface;

	/* Methods  */
	gdouble (*get_page_duration) (PpsDocumentTransition *document_trans,
	                              gint page);
	PpsTransitionEffect *(*get_effect) (PpsDocumentTransition *document_trans,
	                                    gint page);
};

PPS_PUBLIC
gdouble pps_document_transition_get_page_duration (PpsDocumentTransition *document_trans,
                                                   gint page);
PPS_PUBLIC
PpsTransitionEffect *pps_document_transition_get_effect (PpsDocumentTransition *document_trans,
                                                         gint page);

G_END_DECLS
