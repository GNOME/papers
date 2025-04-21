// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-signature.h
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
 */

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#ifndef PPS_SIGNATURE_H
#define PPS_SIGNATURE_H

#include "pps-certificate-info.h"
#include "pps-document.h"
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define PPS_TYPE_SIGNATURE (pps_signature_get_type ())

PPS_PUBLIC
G_DECLARE_FINAL_TYPE (PpsSignature, pps_signature, PPS, SIGNATURE, GObject);

typedef enum {
	PPS_SIGNATURE_STATUS_VALID = 0,
	PPS_SIGNATURE_STATUS_INVALID,
	PPS_SIGNATURE_STATUS_DIGEST_MISMATCH,
	PPS_SIGNATURE_STATUS_DECODING_ERROR,
	PPS_SIGNATURE_STATUS_GENERIC_ERROR
} PpsSignatureStatus;

/* Signature */

struct _PpsSignature {
	GObject base_instance;
};

PPS_PUBLIC
PpsSignature *
pps_signature_new (PpsSignatureStatus status,
                   PpsCertificateInfo *info);

PPS_PUBLIC
void pps_signature_set_destination_file (PpsSignature *self,
                                         const char *file);

PPS_PUBLIC
const char *
pps_signature_get_destination_file (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_page (PpsSignature *self,
                             guint page);

PPS_PUBLIC
gint pps_signature_get_page (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_rect (PpsSignature *self,
                             const PpsRectangle *rect);

PPS_PUBLIC
PpsRectangle *
pps_signature_get_rect (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_signature (PpsSignature *self,
                                  const char *signature);

PPS_PUBLIC
const char *
pps_signature_get_signature (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_signature_left (PpsSignature *self,
                                       const char *signature_left);

PPS_PUBLIC
const char *
pps_signature_get_signature_left (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_font_size (PpsSignature *self,
                                  gint size);

PPS_PUBLIC
gint pps_signature_get_font_size (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_left_font_size (PpsSignature *self,
                                       gint size);

PPS_PUBLIC
gint pps_signature_get_left_font_size (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_border_width (PpsSignature *self,
                                     int width);

PPS_PUBLIC
gint pps_signature_get_border_width (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_password (PpsSignature *self,
                                 const char *password);

PPS_PUBLIC
const char *
pps_signature_get_password (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_font_color (PpsSignature *self,
                                   GdkRGBA *color);

PPS_PUBLIC
void pps_signature_get_font_color (PpsSignature *self,
                                   GdkRGBA *color);

PPS_PUBLIC
void pps_signature_set_border_color (PpsSignature *self,
                                     GdkRGBA *color);

PPS_PUBLIC
void pps_signature_get_border_color (PpsSignature *self,
                                     GdkRGBA *color);

PPS_PUBLIC
void pps_signature_set_background_color (PpsSignature *self,
                                         GdkRGBA *color);

PPS_PUBLIC
void pps_signature_get_background_color (PpsSignature *self,
                                         GdkRGBA *color);

PPS_PUBLIC
void pps_signature_set_owner_password (PpsSignature *self,
                                       const char *password);

PPS_PUBLIC
const char *
pps_signature_get_owner_password (PpsSignature *self);

PPS_PUBLIC
void pps_signature_set_user_password (PpsSignature *self,
                                      const char *password);

PPS_PUBLIC
const char *
pps_signature_get_user_password (PpsSignature *self);

PPS_PUBLIC
gboolean
pps_signature_is_valid (PpsSignature *self);

#endif
