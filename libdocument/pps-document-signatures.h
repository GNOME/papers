/* pps-document-signatures.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2022-2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
 *
 * Papers is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Papers is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <ppsince-document.h> can be included directly."
#endif

#ifndef PPS_DOCUMENT_SIGNATURES_H
#define PPS_DOCUMENT_SIGNATURES_H

#include "pps-document.h"
#include "pps-signature.h"
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define PPS_TYPE_DOCUMENT_SIGNATURES (pps_document_signatures_get_type ())

PPS_PUBLIC
G_DECLARE_INTERFACE (PpsDocumentSignatures, pps_document_signatures, PPS, DOCUMENT_SIGNATURES, GObject)

typedef char *(*PpsSignaturePasswordCallback) (const gchar *text, gpointer user_data);

struct _PpsDocumentSignaturesInterface {
	GTypeInterface base_iface;

	/* Methods  */
	void (*set_password_callback) (PpsDocumentSignatures *document_signatures,
	                               PpsSignaturePasswordCallback cb,
	                               gpointer user_data);
	GList *(*get_available_signing_certificates) (PpsDocumentSignatures *document_signatures);
	PpsCertificateInfo *(*get_certificate_info) (PpsDocumentSignatures *document_signatures,
	                                             const char *nick_name);
	void (*sign) (PpsDocumentSignatures *document_signatures,
	              PpsSignature *signature,
	              GCancellable *cancellable,
	              GAsyncReadyCallback callback,
	              gpointer user_data);
	gboolean (*sign_finish) (PpsDocumentSignatures *document_signatures,
	                         GAsyncResult *result,
	                         GError **error);
	gboolean (*can_sign) (PpsDocumentSignatures *document_signatures);
	GList *(*get_signatures) (PpsDocumentSignatures *document_signatures);
};

PPS_PUBLIC
GList *pps_document_signatures_get_available_signing_certificates (PpsDocumentSignatures *document_signatures);

PPS_PUBLIC
void pps_document_signatures_set_password_callback (PpsDocumentSignatures *document_signatures,
                                                    PpsSignaturePasswordCallback cb,
                                                    gpointer user_data);

PPS_PUBLIC
PpsCertificateInfo *pps_document_signature_get_certificate_info (PpsDocumentSignatures *document_signatures,
                                                                 const char *id);

PPS_PUBLIC
gboolean
pps_document_signatures_sign (PpsDocumentSignatures *document_signatures,
                              PpsSignature *signature,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer user_data);

PPS_PUBLIC
gboolean pps_document_signatures_can_sign (PpsDocumentSignatures *document_signatures);

PPS_PUBLIC
gboolean
pps_document_signatures_sign_finish (PpsDocumentSignatures *document_signatures,
                                     GAsyncResult *result,
                                     GError **error);

PPS_PUBLIC
GList *
pps_document_signatures_get_signatures (PpsDocumentSignatures *document_signatures);

G_END_DECLS

#endif /* PPS_DOCUMENT_SIGNATURES_H */
