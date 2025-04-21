// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-certificate-info.c
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
 * Copyright (C) 2024 Marek Kasik <mkasik@redhat.com>
 */

#include "config.h"

#include "pps-certificate-info.h"
#include "pps-document-signatures.h"
#include "pps-document-type-builtins.h"

enum {
	PROP_0,
	PROP_ID,
	PROP_SUBJECT_COMMON_NAME,
	PROP_SUBJECT_EMAIL,
	PROP_SUBJECT_ORGANIZATION,
	PROP_ISSUER_COMMON_NAME,
	PROP_ISSUER_EMAIL,
	PROP_ISSUER_ORGANIZATION,
	PROP_ISSUANCE_TIME,
	PROP_EXPIRATION_TIME,
	PROP_STATUS
};

typedef struct {
	gchar *id;

	gchar *subject_common_name;
	gchar *subject_email;
	gchar *subject_organization;

	gchar *issuer_common_name;
	gchar *issuer_email;
	gchar *issuer_organization;

	GDateTime *issuance_time;
	GDateTime *expiration_time;

	PpsCertificateStatus status;
} PpsCertificateInfoPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsCertificateInfo, pps_certificate_info, G_TYPE_OBJECT);
#define GET_CERTIFICATE_INFO_PRIVATE(o) pps_certificate_info_get_instance_private (o)

static void
pps_certificate_info_finalize (GObject *object)
{
	PpsCertificateInfo *self = PPS_CERTIFICATE_INFO (object);
	PpsCertificateInfoPrivate *priv = GET_CERTIFICATE_INFO_PRIVATE (self);

	g_clear_pointer (&priv->id, g_free);
	g_clear_pointer (&priv->subject_common_name, g_free);
	g_clear_pointer (&priv->subject_email, g_free);
	g_clear_pointer (&priv->subject_organization, g_free);
	g_clear_pointer (&priv->issuer_common_name, g_free);
	g_clear_pointer (&priv->issuer_email, g_free);
	g_clear_pointer (&priv->issuer_organization, g_free);
	g_clear_pointer (&priv->issuance_time, g_date_time_unref);
	g_clear_pointer (&priv->expiration_time, g_date_time_unref);

	G_OBJECT_CLASS (pps_certificate_info_parent_class)->finalize (object);
}

static void
pps_certificate_info_set_property (GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *param_spec)
{
	PpsCertificateInfo *certificate_info = PPS_CERTIFICATE_INFO (object);
	PpsCertificateInfoPrivate *priv = pps_certificate_info_get_instance_private (certificate_info);

	switch (property_id) {
	case PROP_ID:
		g_free (priv->id);
		priv->id = g_value_dup_string (value);
		break;

	case PROP_SUBJECT_COMMON_NAME:
		priv->subject_common_name = g_value_dup_string (value);
		break;

	case PROP_SUBJECT_EMAIL:
		priv->subject_email = g_value_dup_string (value);
		break;

	case PROP_SUBJECT_ORGANIZATION:
		priv->subject_organization = g_value_dup_string (value);
		break;

	case PROP_ISSUER_COMMON_NAME:
		priv->issuer_common_name = g_value_dup_string (value);
		break;

	case PROP_ISSUER_EMAIL:
		priv->issuer_email = g_value_dup_string (value);
		break;

	case PROP_ISSUER_ORGANIZATION:
		priv->issuer_organization = g_value_dup_string (value);
		break;

	case PROP_ISSUANCE_TIME:
		g_clear_pointer (&priv->issuance_time, g_date_time_unref);
		priv->issuance_time = g_value_get_boxed (value) != NULL ? g_date_time_ref (g_value_get_boxed (value)) : NULL;
		break;

	case PROP_EXPIRATION_TIME:
		g_clear_pointer (&priv->expiration_time, g_date_time_unref);
		priv->expiration_time = g_value_get_boxed (value) != NULL ? g_date_time_ref (g_value_get_boxed (value)) : NULL;
		break;

	case PROP_STATUS:
		priv->status = g_value_get_enum (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
		                                   property_id,
		                                   param_spec);
		break;
	}
}

static void
pps_certificate_info_get_property (GObject *object,
                                   guint property_id,
                                   GValue *value,
                                   GParamSpec *param_spec)
{
	PpsCertificateInfo *certificate_info = PPS_CERTIFICATE_INFO (object);
	PpsCertificateInfoPrivate *priv = pps_certificate_info_get_instance_private (certificate_info);

	switch (property_id) {
	case PROP_ID:
		g_value_set_string (value, priv->id);
		break;

	case PROP_SUBJECT_COMMON_NAME:
		g_value_set_string (value, priv->subject_common_name);
		break;

	case PROP_SUBJECT_EMAIL:
		g_value_set_string (value, priv->subject_email);
		break;

	case PROP_SUBJECT_ORGANIZATION:
		g_value_set_string (value, priv->subject_organization);
		break;

	case PROP_ISSUER_COMMON_NAME:
		g_value_set_string (value, priv->issuer_common_name);
		break;

	case PROP_ISSUER_EMAIL:
		g_value_set_string (value, priv->issuer_email);
		break;

	case PROP_ISSUER_ORGANIZATION:
		g_value_set_string (value, priv->issuer_organization);
		break;

	case PROP_ISSUANCE_TIME:
		g_value_set_boxed (value, g_date_time_ref (priv->issuance_time));
		break;

	case PROP_EXPIRATION_TIME:
		g_value_set_boxed (value, g_date_time_ref (priv->expiration_time));
		break;

	case PROP_STATUS:
		g_value_set_enum (value, priv->status);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
		                                   property_id,
		                                   param_spec);
		break;
	}
}

static void
pps_certificate_info_init (PpsCertificateInfo *self)
{
	PpsCertificateInfoPrivate *priv = GET_CERTIFICATE_INFO_PRIVATE (self);

	priv->status = PPS_CERTIFICATE_STATUS_NOT_VERIFIED;
}

static void
pps_certificate_info_class_init (PpsCertificateInfoClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->set_property = pps_certificate_info_set_property;
	g_object_class->get_property = pps_certificate_info_get_property;

	/* Properties */
	g_object_class_install_property (g_object_class,
	                                 PROP_ID,
	                                 g_param_spec_string ("id",
	                                                      "Id",
	                                                      "Id of the certificate",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_SUBJECT_COMMON_NAME,
	                                 g_param_spec_string ("subject-common-name",
	                                                      "Subject's Common Name",
	                                                      "The name of the entity owning the certificate",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_SUBJECT_EMAIL,
	                                 g_param_spec_string ("subject-email",
	                                                      "Email",
	                                                      "The email of the entity that signed the document",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_SUBJECT_ORGANIZATION,
	                                 g_param_spec_string ("subject-organization",
	                                                      "Organization",
	                                                      "The organization of the entity that owns the document",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_ISSUER_COMMON_NAME,
	                                 g_param_spec_string ("issuer-common-name",
	                                                      "Certificate Issuer's Common Name",
	                                                      "The name of the entity that issued the certificate",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_ISSUER_EMAIL,
	                                 g_param_spec_string ("issuer-email",
	                                                      "Certificate Issuer's Email",
	                                                      "The email of the issuer of the certificate",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_ISSUER_ORGANIZATION,
	                                 g_param_spec_string ("issuer-organization",
	                                                      "Certificate Issuer's Organization Name",
	                                                      "The name of the organization that issued the certificate",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_ISSUANCE_TIME,
	                                 g_param_spec_boxed ("issuance-time",
	                                                     "IssuanceTime",
	                                                     "The time when the certificate was issued",
	                                                     G_TYPE_DATE_TIME,
	                                                     G_PARAM_READWRITE |
	                                                         G_PARAM_CONSTRUCT_ONLY |
	                                                         G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_EXPIRATION_TIME,
	                                 g_param_spec_boxed ("expiration-time",
	                                                     "ExpirationTime",
	                                                     "The time when the certificate expires",
	                                                     G_TYPE_DATE_TIME,
	                                                     G_PARAM_READWRITE |
	                                                         G_PARAM_CONSTRUCT_ONLY |
	                                                         G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_STATUS,
	                                 g_param_spec_enum ("status",
	                                                    "Status",
	                                                    "Status of the certificate",
	                                                    PPS_TYPE_CERTIFICATE_STATUS,
	                                                    PPS_CERTIFICATE_STATUS_NOT_VERIFIED,
	                                                    G_PARAM_READWRITE |
	                                                        G_PARAM_CONSTRUCT_ONLY |
	                                                        G_PARAM_STATIC_STRINGS));

	g_object_class->finalize = pps_certificate_info_finalize;
}
