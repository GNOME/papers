// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2006 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#include <config.h>

#include <glib/gstdio.h>
#include <unistd.h>

#include "pps-document-misc.h"
#include "pps-file-helpers.h"
#include "pps-image.h"

struct _PpsImagePrivate {
	gint page;
	gint id;
	GdkPixbuf *pixbuf;
	gchar *tmp_uri;
};

typedef struct _PpsImagePrivate PpsImagePrivate;

#define GET_PRIVATE(o) pps_image_get_instance_private (o);

G_DEFINE_TYPE_WITH_PRIVATE (PpsImage, pps_image, G_TYPE_OBJECT)

static void
pps_image_finalize (GObject *object)
{
	PpsImage *image = PPS_IMAGE (object);
	PpsImagePrivate *priv = GET_PRIVATE (image);

	g_clear_object (&priv->pixbuf);

	if (priv->tmp_uri) {
		gchar *filename;

		filename = g_filename_from_uri (priv->tmp_uri, NULL, NULL);
		pps_tmp_filename_unlink (filename);
		g_free (filename);
		g_clear_pointer (&priv->tmp_uri, g_free);
	}

	(*G_OBJECT_CLASS (pps_image_parent_class)->finalize) (object);
}

static void
pps_image_class_init (PpsImageClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->finalize = pps_image_finalize;
}

static void
pps_image_init (PpsImage *image)
{
}

PpsImage *
pps_image_new (gint page,
               gint img_id)
{
	PpsImage *image;
	PpsImagePrivate *priv;

	image = PPS_IMAGE (g_object_new (PPS_TYPE_IMAGE, NULL));

	priv = GET_PRIVATE (image);
	priv->page = page;
	priv->id = img_id;

	return image;
}

PpsImage *
pps_image_new_from_pixbuf (GdkPixbuf *pixbuf)
{
	PpsImage *image;
	PpsImagePrivate *priv;

	g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

	image = PPS_IMAGE (g_object_new (PPS_TYPE_IMAGE, NULL));
	priv = GET_PRIVATE (image);
	priv->pixbuf = g_object_ref (pixbuf);

	return image;
}

gint
pps_image_get_page (PpsImage *image)
{
	g_return_val_if_fail (PPS_IS_IMAGE (image), -1);
	PpsImagePrivate *priv = GET_PRIVATE (image);

	return priv->page;
}

gint
pps_image_get_id (PpsImage *image)
{
	g_return_val_if_fail (PPS_IS_IMAGE (image), -1);
	PpsImagePrivate *priv = GET_PRIVATE (image);

	return priv->id;
}

/**
 * pps_image_get_pixbuf:
 * @image: an #PpsImage
 *
 * Returns: (transfer none): a #GdkPixbuf
 */
GdkPixbuf *
pps_image_get_pixbuf (PpsImage *image)
{
	PpsImagePrivate *priv;
	g_return_val_if_fail (PPS_IS_IMAGE (image), NULL);
	priv = GET_PRIVATE (image);
	g_return_val_if_fail (GDK_IS_PIXBUF (priv->pixbuf), NULL);

	return priv->pixbuf;
}

const gchar *
pps_image_save_tmp (PpsImage *image,
                    GdkPixbuf *pixbuf)
{
	GError *error = NULL;
	gchar *filename = NULL;
	PpsImagePrivate *priv;
	int fd;

	g_return_val_if_fail (PPS_IS_IMAGE (image), NULL);
	g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

	priv = GET_PRIVATE (image);

	if (priv->tmp_uri)
		return priv->tmp_uri;

	if ((fd = pps_mkstemp ("image.XXXXXX.png", &filename, &error)) == -1)
		goto had_error;

	gdk_pixbuf_save (pixbuf, filename,
	                 "png", &error,
	                 "compression", "3", NULL);
	close (fd);

	if (!error) {
		priv->tmp_uri = g_filename_to_uri (filename, NULL, &error);
		if (priv->tmp_uri == NULL)
			goto had_error;

		g_free (filename);

		return priv->tmp_uri;
	}

had_error:

	/* Erro saving image */
	g_warning ("Error saving image: %s", error->message);
	g_error_free (error);
	g_free (filename);

	return NULL;
}

const gchar *
pps_image_get_tmp_uri (PpsImage *image)
{
	g_return_val_if_fail (PPS_IS_IMAGE (image), NULL);
	PpsImagePrivate *priv = GET_PRIVATE (image);

	return priv->tmp_uri;
}
