// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-security.h
 *  this file is part of papers, a gnome pdf viewer
 *
 * Copyright (C) 2005 Red Hat, Inc.
 *
 * Author:
 *   Jonathan Blandford <jrb@alum.mit.edu>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib-object.h>
#include <glib.h>

#include "pps-document.h"
#include "pps-macros.h"

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_SECURITY (pps_document_security_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentSecurity, pps_document_security, PPS, DOCUMENT_SECURITY, GObject)

struct _PpsDocumentSecurityInterface {
	GTypeInterface base_iface;

	/* Methods  */
	gboolean (*has_document_security) (PpsDocumentSecurity *document_security);
	void (*set_password) (PpsDocumentSecurity *document_security,
	                      const char *password);
};

PPS_PUBLIC
gboolean pps_document_security_has_document_security (PpsDocumentSecurity *document_security);
PPS_PUBLIC
void pps_document_security_set_password (PpsDocumentSecurity *document_security,
                                         const char *password);

G_END_DECLS
