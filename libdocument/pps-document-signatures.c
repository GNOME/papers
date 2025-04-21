// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-signatures.c
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2022-2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
 */

#include "config.h"

#include "pps-document-signatures.h"

G_DEFINE_INTERFACE (PpsDocumentSignatures, pps_document_signatures, 0)

static void
pps_document_signatures_default_init (PpsDocumentSignaturesInterface *klass)
{
}

/**
 * pps_document_signatures_set_password_callback:
 * @document_signatures: an #PpsDocumentSignatures
 * @cb: (scope forever): (closure user_data) an #PpsSignaturePasswordCallback
 * @user_data: data to pass to @cb
 *
 * Popplers crypto backend asks for certificate password before access,
 * so set a helper function to actually allow the user to enter his password.
 */
void
pps_document_signatures_set_password_callback (PpsDocumentSignatures *document_signatures,
                                               PpsSignaturePasswordCallback cb,
                                               gpointer user_data)
{
	PpsDocumentSignaturesInterface *iface = PPS_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);

	if (iface->set_password_callback)
		iface->set_password_callback (document_signatures, cb, user_data);
}

/**
 * pps_document_signatures_get_available_signing_certificates:
 * @document_signatures: an #PpsDocumentSignatures
 *
 * Returns: (transfer full) (element-type PpsCertificateInfo): a list of #PpsCertificateInfo objects or %NULL
 */
GList *
pps_document_signatures_get_available_signing_certificates (PpsDocumentSignatures *document_signatures)
{
	PpsDocumentSignaturesInterface *iface = PPS_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);

	if (iface->get_available_signing_certificates)
		return iface->get_available_signing_certificates (document_signatures);

	return NULL;
}

/**
 * pps_document_signatures_get_certificate_info:
 * @document_signatures: an #PpsDocumentSignatures
 * @nick_name: certificate nick name
 *
 * Returns: (transfer full): a new #PpsCertificateInfo, or %NULL
 */
PpsCertificateInfo *
pps_document_signatures_get_certificate_info (PpsDocumentSignatures *document_signatures,
                                              const char *nick_name)
{
	PpsDocumentSignaturesInterface *iface = PPS_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);

	if (iface->get_certificate_info)
		return iface->get_certificate_info (document_signatures, nick_name);

	return NULL;
}

/**
 * pps_document_signatures_sign:
 * @document_signatures: an #PpsDocumentSignatures
 * @signature: a #PpsSignature
 * @cancellable: a #GCancellable
 * @callback: a #GAsyncReadyCallback
 * @user_data: callback user data
 *
 * Start the final async process of signing a document using the prepared signature.
 *
 * Returns: %TRUE if signing has been started,otherwise %FALSE
 */
gboolean
pps_document_signatures_sign (PpsDocumentSignatures *document_signatures,
                              PpsSignature *signature,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer user_data)
{
	PpsDocumentSignaturesInterface *iface = PPS_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);

	if (iface->sign) {
		iface->sign (document_signatures, signature, cancellable, callback, user_data);
		return TRUE;
	}

	return FALSE;
}

/**
 * pps_document_signatures_sign_finish:
 * @document_signatures: a #PpsDocumentSignatures
 * @result: a #GAsyncResult
 * @error: a #Gerror
 *
 * Finish `pps_document_signatures_sign` operation,
 *
 * Returns: %TRUE if signing finish has been started,otherwise %FALSE
 */
gboolean
pps_document_signatures_sign_finish (PpsDocumentSignatures *document_signatures,
                                     GAsyncResult *result,
                                     GError **error)
{
	PpsDocumentSignaturesInterface *iface = PPS_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);

	if (iface->sign_finish) {
		iface->sign_finish (document_signatures, result, error);
		return TRUE;
	}

	return FALSE;
}

/**
 * pps_document_signatures_can_sign:
 * @document_signatures: a #PpsDocumentSignatures
 *
 * Checks whether document supports digital signing.
 *
 * Returns: %TRUE if signing is supported,otherwise %FALSE
 */
gboolean
pps_document_signatures_can_sign (PpsDocumentSignatures *document_signatures)
{
	PpsDocumentSignaturesInterface *iface = PPS_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);

	if (iface->can_sign)
		return iface->can_sign (document_signatures);

	return FALSE;
}

/**
 * pps_document_signatures_has_signatures:
 * @document_signatures: an #PpsDocumentSignatures
 *
 * Returns: if the document has digital signatures
 */
gboolean
pps_document_signatures_has_signatures (PpsDocumentSignatures *document_signatures)
{
	PpsDocumentSignaturesInterface *iface = PPS_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);

	if (iface->has_signatures)
		return iface->has_signatures (document_signatures);

	return FALSE;
}

/**
 * pps_document_signatures_get_signatures:
 * @document_signatures: an #PpsDocumentSignatures
 *
 * Returns: (element-type PpsSignature) (transfer full): a list of #PpsSignature objects
 */
GList *
pps_document_signatures_get_signatures (PpsDocumentSignatures *document_signatures)
{
	PpsDocumentSignaturesInterface *iface = PPS_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);

	if (iface->get_signatures)
		return iface->get_signatures (document_signatures);

	return NULL;
}
