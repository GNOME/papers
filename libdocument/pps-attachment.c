// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2006 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#include "pps-attachment.h"
#include "pps-file-helpers.h"
#include <config.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

enum {
	PROP_0,
	PROP_NAME,
	PROP_DESCRIPTION,
	PROP_MDATETIME,
	PROP_CDATETIME,
	PROP_SIZE,
	PROP_DATA
};

typedef struct
{
	gchar *name;
	gchar *description;
	GDateTime *mdatetime;
	GDateTime *cdatetime;
	gsize size;
	gchar *data;
	gchar *mime_type;

	GAppInfo *app;
	GFile *tmp_file;
} PpsAttachmentPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsAttachment, pps_attachment, G_TYPE_OBJECT)

#define GET_PRIVATE(o) pps_attachment_get_instance_private (o);

GQuark
pps_attachment_error_quark (void)
{
	static GQuark error_quark = 0;

	if (error_quark == 0)
		error_quark =
		    g_quark_from_static_string ("pps-attachment-error-quark");

	return error_quark;
}

static void
pps_attachment_finalize (GObject *object)
{
	PpsAttachment *attachment = PPS_ATTACHMENT (object);
	PpsAttachmentPrivate *priv = GET_PRIVATE (attachment);

	g_clear_pointer (&priv->name, g_free);
	g_clear_pointer (&priv->description, g_free);
	g_clear_pointer (&priv->data, g_free);
	g_clear_pointer (&priv->mime_type, g_free);
	g_clear_object (&priv->app);

	g_clear_pointer (&priv->mdatetime, g_date_time_unref);
	g_clear_pointer (&priv->cdatetime, g_date_time_unref);

	if (priv->tmp_file) {
		pps_tmp_file_unlink (priv->tmp_file);
		g_clear_object (&priv->tmp_file);
	}

	G_OBJECT_CLASS (pps_attachment_parent_class)->finalize (object);
}

static void
pps_attachment_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *param_spec)
{
	PpsAttachment *attachment = PPS_ATTACHMENT (object);
	PpsAttachmentPrivate *priv = GET_PRIVATE (attachment);

	switch (prop_id) {
	case PROP_NAME:
		priv->name = g_value_dup_string (value);
		break;
	case PROP_DESCRIPTION:
		priv->description = g_value_dup_string (value);
		break;
	case PROP_MDATETIME:
		priv->mdatetime = g_value_get_boxed (value);
		if (priv->mdatetime)
			g_date_time_ref (priv->mdatetime);
		break;
	case PROP_CDATETIME:
		priv->cdatetime = g_value_get_boxed (value);
		if (priv->cdatetime)
			g_date_time_ref (priv->cdatetime);
		break;
	case PROP_SIZE:
		priv->size = g_value_get_uint (value);
		break;
	case PROP_DATA:
		priv->data = g_value_get_pointer (value);
		priv->mime_type = g_content_type_guess (priv->name,
		                                        (guchar *) priv->data,
		                                        priv->size,
		                                        NULL);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
		                                   prop_id,
		                                   param_spec);
		break;
	}
}

static void
pps_attachment_class_init (PpsAttachmentClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->set_property = pps_attachment_set_property;

	/* Properties */
	g_object_class_install_property (g_object_class,
	                                 PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name",
	                                                      "The attachment name",
	                                                      NULL,
	                                                      G_PARAM_WRITABLE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_DESCRIPTION,
	                                 g_param_spec_string ("description",
	                                                      "Description",
	                                                      "The attachment description",
	                                                      NULL,
	                                                      G_PARAM_WRITABLE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_MDATETIME,
	                                 g_param_spec_boxed ("mdatetime",
	                                                     "ModifiedTime",
	                                                     "The attachment modification date",
	                                                     G_TYPE_DATE_TIME,
	                                                     G_PARAM_WRITABLE |
	                                                         G_PARAM_CONSTRUCT_ONLY |
	                                                         G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_CDATETIME,
	                                 g_param_spec_boxed ("cdatetime",
	                                                     "CreationTime",
	                                                     "The attachment creation date",
	                                                     G_TYPE_DATE_TIME,
	                                                     G_PARAM_WRITABLE |
	                                                         G_PARAM_CONSTRUCT_ONLY |
	                                                         G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_SIZE,
	                                 g_param_spec_uint ("size",
	                                                    "Size",
	                                                    "The attachment size",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_WRITABLE |
	                                                        G_PARAM_CONSTRUCT_ONLY |
	                                                        G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_DATA,
	                                 g_param_spec_pointer ("data",
	                                                       "Data",
	                                                       "The attachment data",
	                                                       G_PARAM_WRITABLE |
	                                                           G_PARAM_CONSTRUCT_ONLY |
	                                                           G_PARAM_STATIC_STRINGS));

	g_object_class->finalize = pps_attachment_finalize;
}

static void
pps_attachment_init (PpsAttachment *attachment)
{
}

PpsAttachment *
pps_attachment_new (const gchar *name,
                    const gchar *description,
                    GDateTime *mdatetime,
                    GDateTime *cdatetime,
                    gsize size,
                    gpointer data)
{
	return (PpsAttachment *) g_object_new (PPS_TYPE_ATTACHMENT,
	                                       "name", name,
	                                       "description", description,
	                                       "mdatetime", mdatetime,
	                                       "cdatetime", cdatetime,
	                                       "size", size,
	                                       "data", data,
	                                       NULL);
}

const gchar *
pps_attachment_get_name (PpsAttachment *attachment)
{
	PpsAttachmentPrivate *priv;

	g_return_val_if_fail (PPS_IS_ATTACHMENT (attachment), NULL);

	priv = GET_PRIVATE (attachment);

	return priv->name;
}

const gchar *
pps_attachment_get_description (PpsAttachment *attachment)
{
	PpsAttachmentPrivate *priv;

	g_return_val_if_fail (PPS_IS_ATTACHMENT (attachment), NULL);

	priv = GET_PRIVATE (attachment);

	return priv->description;
}

GDateTime *
pps_attachment_get_modification_datetime (PpsAttachment *attachment)
{
	PpsAttachmentPrivate *priv;

	g_return_val_if_fail (PPS_IS_ATTACHMENT (attachment), 0);

	priv = GET_PRIVATE (attachment);

	return priv->mdatetime;
}

GDateTime *
pps_attachment_get_creation_datetime (PpsAttachment *attachment)
{
	PpsAttachmentPrivate *priv;

	g_return_val_if_fail (PPS_IS_ATTACHMENT (attachment), 0);

	priv = GET_PRIVATE (attachment);

	return priv->cdatetime;
}

const gchar *
pps_attachment_get_mime_type (PpsAttachment *attachment)
{
	PpsAttachmentPrivate *priv;

	g_return_val_if_fail (PPS_IS_ATTACHMENT (attachment), NULL);

	priv = GET_PRIVATE (attachment);

	return priv->mime_type;
}

gboolean
pps_attachment_save (PpsAttachment *attachment,
                     GFile *file,
                     GError **error)
{
	g_autoptr (GFileOutputStream) output_stream = NULL;
	g_autoptr (GError) ioerror = NULL;
	gssize written_bytes;
	PpsAttachmentPrivate *priv = GET_PRIVATE (attachment);

	g_return_val_if_fail (PPS_IS_ATTACHMENT (attachment), FALSE);
	g_return_val_if_fail (G_IS_FILE (file), FALSE);

	output_stream = g_file_replace (file, NULL, FALSE, 0, NULL, &ioerror);
	if (output_stream == NULL) {
		g_autofree char *uri = g_file_get_uri (file);
		g_set_error (error,
		             PPS_ATTACHMENT_ERROR,
		             ioerror->code,
		             _ ("Couldn’t save attachment “%s”: %s"),
		             uri,
		             ioerror->message);

		return FALSE;
	}

	written_bytes = g_output_stream_write (G_OUTPUT_STREAM (output_stream),
	                                       priv->data,
	                                       priv->size,
	                                       NULL, &ioerror);
	if (written_bytes == -1) {
		g_autofree char *uri = g_file_get_uri (file);
		g_set_error (error,
		             PPS_ATTACHMENT_ERROR,
		             ioerror->code,
		             _ ("Couldn’t save attachment “%s”: %s"),
		             uri,
		             ioerror->message);

		return FALSE;
	}

	return TRUE;
}

static gboolean
pps_attachment_launch_app (PpsAttachment *attachment,
                           GAppLaunchContext *context,
                           GError **error)
{
	gboolean result;
	GList *files = NULL;
	GError *ioerror = NULL;
	PpsAttachmentPrivate *priv = GET_PRIVATE (attachment);

	g_assert (G_IS_FILE (priv->tmp_file));
	g_assert (G_IS_APP_INFO (priv->app));

	files = g_list_prepend (files, priv->tmp_file);

	result = g_app_info_launch (priv->app, files, context, &ioerror);

	if (!result) {
		g_set_error (error,
		             PPS_ATTACHMENT_ERROR,
		             (gint) result,
		             _ ("Couldn’t open attachment “%s”: %s"),
		             priv->name,
		             ioerror->message);

		g_list_free (files);
		g_error_free (ioerror);

		return FALSE;
	}

	g_list_free (files);

	return TRUE;
}

gboolean
pps_attachment_open (PpsAttachment *attachment,
                     GAppLaunchContext *context,
                     GError **error)
{
	GAppInfo *app_info;
	gboolean retval = FALSE;
	PpsAttachmentPrivate *priv;

	g_return_val_if_fail (PPS_IS_ATTACHMENT (attachment), FALSE);

	priv = GET_PRIVATE (attachment);

	if (!priv->app) {
		app_info = g_app_info_get_default_for_type (priv->mime_type, FALSE);
		priv->app = app_info;
	}

	if (!priv->app) {
		g_set_error (error,
		             PPS_ATTACHMENT_ERROR,
		             0,
		             _ ("Couldn’t open attachment “%s”"),
		             priv->name);

		return FALSE;
	}

	if (priv->tmp_file) {
		retval = pps_attachment_launch_app (attachment, context, error);
	} else {
		char *basename;
		char *temp_dir;
		char *file_path;
		GFile *file;

		/* FIXMEchpe: convert to filename encoding first!
		 * Store the file inside a temporary XXXXXX subdirectory to
		 * keep the filename "as is".
		 */
		basename = g_path_get_basename (pps_attachment_get_name (attachment));
		temp_dir = g_dir_make_tmp ("papers.XXXXXX", error);
		file_path = g_build_filename (temp_dir, basename, NULL);
		file = g_file_new_for_path (file_path);

		g_free (temp_dir);
		g_free (file_path);
		g_free (basename);

		if (file != NULL && pps_attachment_save (attachment, file, error)) {
			g_set_object (&priv->tmp_file, file);

			retval = pps_attachment_launch_app (attachment, context, error);
		}

		g_object_unref (file);
	}

	return retval;
}
