// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-certificate-info.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
 * Copyright (C) 2024 Marek Kasik <mkasik@redhat.com>
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include "pps-document.h"

G_BEGIN_DECLS

#define PPS_TYPE_CERTIFICATE_INFO (pps_certificate_info_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsCertificateInfo, pps_certificate_info, PPS, CERTIFICATE_INFO, GObject);

struct _PpsCertificateInfo {
	GObject base_instance;
};

typedef enum {
	PPS_CERTIFICATE_STATUS_TRUSTED = 0,
	PPS_CERTIFICATE_STATUS_UNTRUSTED_ISSUER,
	PPS_CERTIFICATE_STATUS_UNKNOWN_ISSUER,
	PPS_CERTIFICATE_STATUS_REVOKED,
	PPS_CERTIFICATE_STATUS_EXPIRED,
	PPS_CERTIFICATE_STATUS_GENERIC_ERROR,
	PPS_CERTIFICATE_STATUS_NOT_VERIFIED
} PpsCertificateStatus;

PPS_PUBLIC
PpsCertificateInfo *pps_certificate_info_new (const char *id,
                                              const char *subject_common_name);
