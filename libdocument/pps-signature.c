// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-signature.c
 * this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Jan-Michael Brummer <jan-michael.brummer1@volkswagen.de>
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "pps-document-signatures.h"
#include "pps-document-type-builtins.h"
#include "pps-signature.h"

typedef struct
{
	char *destination_file;
	PpsCertificateInfo *certificate_info;
	char *password;
	int page;
	char *signature;
	char *signature_left;

	PpsRectangle *rect;
	GdkRGBA font_color;
	GdkRGBA border_color;
	GdkRGBA background_color;
	gdouble font_size;
	gdouble left_font_size;
	gdouble border_width;
	char *document_owner_password;
	char *document_user_password;

	PpsSignatureStatus signature_status;
	GDateTime *signature_time;
} PpsSignaturePrivate;

enum {
	PROP_0,
	PROP_STATUS,
	PROP_SIGN_TIME,
	PROP_CERTIFICATE_INFO
};

G_DEFINE_TYPE_WITH_PRIVATE (PpsSignature, pps_signature, G_TYPE_OBJECT);
#define GET_SIG_PRIVATE(o) pps_signature_get_instance_private (o)

static void
pps_signature_finalize (GObject *object)
{
	PpsSignature *self = PPS_SIGNATURE (object);
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	g_clear_object (&priv->certificate_info);
	g_clear_pointer (&priv->destination_file, g_free);
	g_clear_pointer (&priv->password, g_free);
	g_clear_pointer (&priv->signature, g_free);
	g_clear_pointer (&priv->signature_left, g_free);
	g_clear_pointer (&priv->rect, g_free);
	g_clear_pointer (&priv->document_owner_password, g_free);
	g_clear_pointer (&priv->document_user_password, g_free);
	g_clear_pointer (&priv->signature_time, g_date_time_unref);

	G_OBJECT_CLASS (pps_signature_parent_class)->finalize (object);
}

static void
pps_signature_init (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	gdk_rgba_parse (&priv->font_color, "#000000");
	gdk_rgba_parse (&priv->border_color, "#000000");
	gdk_rgba_parse (&priv->background_color, "#F0F0F0");

	priv->font_size = 10.0;
	priv->left_font_size = 20.0;
	priv->border_width = 1.5;
	priv->certificate_info = NULL;
}

static void
pps_signature_set_property (GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *param_spec)
{
	PpsSignature *self = PPS_SIGNATURE (object);
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	switch (property_id) {
	case PROP_STATUS:
		priv->signature_status = g_value_get_enum (value);
		break;

	case PROP_SIGN_TIME:
		priv->signature_time = g_value_get_boxed (value);
		if (priv->signature_time)
			g_date_time_ref (priv->signature_time);
		break;

	case PROP_CERTIFICATE_INFO:
		g_clear_object (&priv->certificate_info);
		priv->certificate_info = g_object_ref (g_value_get_object (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
		                                   property_id,
		                                   param_spec);
		break;
	}
}

static void
pps_signature_get_property (GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *param_spec)
{
	PpsSignature *self = PPS_SIGNATURE (object);
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	switch (property_id) {
	case PROP_STATUS:
		g_value_set_enum (value, priv->signature_status);
		break;

	case PROP_SIGN_TIME:
		g_value_set_boxed (value, g_date_time_ref (priv->signature_time));
		break;

	case PROP_CERTIFICATE_INFO:
		g_value_set_object (value, priv->certificate_info);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
		                                   property_id,
		                                   param_spec);
		break;
	}
}

static void
pps_signature_class_init (PpsSignatureClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->finalize = pps_signature_finalize;
	g_object_class->set_property = pps_signature_set_property;
	g_object_class->get_property = pps_signature_get_property;

	/* Properties */
	g_object_class_install_property (g_object_class,
	                                 PROP_STATUS,
	                                 g_param_spec_enum ("status",
	                                                    "Status",
	                                                    "Status of the signature",
	                                                    PPS_TYPE_SIGNATURE_STATUS,
	                                                    PPS_SIGNATURE_STATUS_INVALID,
	                                                    G_PARAM_READWRITE |
	                                                        G_PARAM_CONSTRUCT_ONLY |
	                                                        G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_SIGN_TIME,
	                                 g_param_spec_boxed ("signature-time",
	                                                     "SignatureTime",
	                                                     "The time associated with the signature",
	                                                     G_TYPE_DATE_TIME,
	                                                     G_PARAM_READWRITE |
	                                                         G_PARAM_CONSTRUCT_ONLY |
	                                                         G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_CERTIFICATE_INFO,
	                                 g_param_spec_object ("certificate-info",
	                                                      "CertificateInfo",
	                                                      "Information about certificate used for the signature",
	                                                      PPS_TYPE_CERTIFICATE_INFO,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
}

void
pps_signature_set_destination_file (PpsSignature *self,
                                    const char *file)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	g_clear_pointer (&priv->destination_file, g_free);
	priv->destination_file = g_strdup (file);
}

const char *
pps_signature_get_destination_file (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->destination_file;
}

void
pps_signature_set_page (PpsSignature *self,
                        guint page)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	priv->page = page;
}

gint
pps_signature_get_page (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->page;
}

void
pps_signature_set_rect (PpsSignature *self,
                        const PpsRectangle *rectangle)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	g_clear_pointer (&priv->rect, g_free);
	priv->rect = pps_rectangle_copy ((PpsRectangle *) rectangle);
}

PpsRectangle *
pps_signature_get_rect (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->rect;
}

void
pps_signature_set_signature (PpsSignature *self,
                             const char *signature)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	g_clear_pointer (&priv->signature, g_free);
	priv->signature = g_strdup (signature);
}

const char *
pps_signature_get_signature (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->signature;
}

void
pps_signature_set_signature_left (PpsSignature *self,
                                  const char *signature_left)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	g_clear_pointer (&priv->signature_left, g_free);
	priv->signature_left = g_strdup (signature_left);
}

const char *
pps_signature_get_signature_left (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->signature_left;
}

const char *
pps_signature_get_password (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->password;
}

void
pps_signature_set_font_color (PpsSignature *self,
                              GdkRGBA *color)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	priv->font_color.red = color->red;
	priv->font_color.green = color->green;
	priv->font_color.blue = color->blue;
	priv->font_color.alpha = color->alpha;
}

void
pps_signature_get_font_color (PpsSignature *self,
                              GdkRGBA *color)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	color->red = priv->font_color.red;
	color->green = priv->font_color.green;
	color->blue = priv->font_color.blue;
	color->alpha = priv->font_color.alpha;
}

void
pps_signature_set_border_color (PpsSignature *self,
                                GdkRGBA *color)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	priv->border_color.red = color->red;
	priv->border_color.green = color->green;
	priv->border_color.blue = color->blue;
	priv->border_color.alpha = color->alpha;
}

void
pps_signature_get_border_color (PpsSignature *self,
                                GdkRGBA *color)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	color->red = priv->border_color.red;
	color->green = priv->border_color.green;
	color->blue = priv->border_color.blue;
	color->alpha = priv->border_color.alpha;
}

void
pps_signature_set_background_color (PpsSignature *self,
                                    GdkRGBA *color)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	priv->background_color.red = color->red;
	priv->background_color.green = color->green;
	priv->background_color.blue = color->blue;
	priv->background_color.alpha = color->alpha;
}

void
pps_signature_get_background_color (PpsSignature *self,
                                    GdkRGBA *color)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	color->red = priv->background_color.red;
	color->green = priv->background_color.green;
	color->blue = priv->background_color.blue;
	color->alpha = priv->background_color.alpha;
}

void
pps_signature_set_owner_password (PpsSignature *self,
                                  const char *password)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	g_clear_pointer (&priv->document_owner_password, g_free);
	priv->document_owner_password = g_strdup (password);
}

const char *
pps_signature_get_owner_password (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->document_owner_password;
}

void
pps_signature_set_user_password (PpsSignature *self,
                                 const char *password)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	g_clear_pointer (&priv->document_user_password, g_free);
	priv->document_user_password = g_strdup (password);
}

const char *
pps_signature_get_user_password (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->document_user_password;
}

void
pps_signature_set_font_size (PpsSignature *self,
                             gint size)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	priv->font_size = size;
}

gint
pps_signature_get_font_size (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->font_size;
}

void
pps_signature_set_left_font_size (PpsSignature *self,
                                  gint size)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	priv->left_font_size = size;
}

gint
pps_signature_get_left_font_size (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->left_font_size;
}

void
pps_signature_set_border_width (PpsSignature *self,
                                gint width)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	priv->border_width = width;
}

gint
pps_signature_get_border_width (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);

	return priv->border_width;
}

/**
 * pps_signature_is_valid:
 * @self: a #PpsSignature
 *
 * Checks whether signature is valid
 *
 * Returns: signature valid status
 */
gboolean
pps_signature_is_valid (PpsSignature *self)
{
	PpsSignaturePrivate *priv = GET_SIG_PRIVATE (self);
	g_autoptr (PpsCertificateInfo) certificate_info = NULL;
	PpsCertificateStatus certificate_status = PPS_CERTIFICATE_STATUS_NOT_VERIFIED;

	g_object_get (self, "certificate-info", &certificate_info, NULL);
	if (certificate_info != NULL)
		g_object_get (certificate_info, "status", &certificate_status, NULL);

	return certificate_status == PPS_CERTIFICATE_STATUS_TRUSTED && priv->signature_status == PPS_SIGNATURE_STATUS_VALID;
}

PpsSignature *
pps_signature_new (PpsSignatureStatus status, PpsCertificateInfo *info)
{
	return g_object_new (PPS_TYPE_SIGNATURE,
	                     "status", status,
	                     "certificate-info", info,
	                     NULL);
}
