// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-links.h
 *  this file is part of papers, a gnome document_links viewer
 *
 * Copyright (C) 2004 Red Hat, Inc.
 */

#include "config.h"

#include "pps-document-security.h"

G_DEFINE_INTERFACE (PpsDocumentSecurity, pps_document_security, 0)

static void
pps_document_security_default_init (PpsDocumentSecurityInterface *klass)
{
}

gboolean
pps_document_security_has_document_security (PpsDocumentSecurity *document_security)
{
	PpsDocumentSecurityInterface *iface = PPS_DOCUMENT_SECURITY_GET_IFACE (document_security);
	return iface->has_document_security (document_security);
}

void
pps_document_security_set_password (PpsDocumentSecurity *document_security,
                                    const char *password)
{
	PpsDocumentSecurityInterface *iface = PPS_DOCUMENT_SECURITY_GET_IFACE (document_security);
	iface->set_password (document_security, password);
}
