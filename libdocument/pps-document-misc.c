// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2009 Juanjo Marín <juanj.marin@juntadeandalucia.es>
 *  Copyright (c) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
 *  Copyright (C) 2000-2003 Marco Pesenti Gritti
 */

#include <config.h>

#include <gtk/gtk.h>
#include <string.h>

#include "pps-document-misc.h"

cairo_surface_t *
pps_document_misc_surface_from_pixbuf (GdkPixbuf *pixbuf)
{
	cairo_surface_t *surface;
	cairo_t *cr;

	g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

	surface = cairo_image_surface_create (gdk_pixbuf_get_has_alpha (pixbuf) ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
	                                      gdk_pixbuf_get_width (pixbuf),
	                                      gdk_pixbuf_get_height (pixbuf));
	cr = cairo_create (surface);

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
	G_GNUC_END_IGNORE_DEPRECATIONS

	cairo_paint (cr);
	cairo_destroy (cr);

	return surface;
}

/**
 * pps_document_misc_pixbuf_from_surface:
 * @surface: a #cairo_surface_t
 *
 * Returns: (transfer full): a #GdkPixbuf
 */
GdkPixbuf *
pps_document_misc_pixbuf_from_surface (cairo_surface_t *surface)
{
	g_return_val_if_fail (surface, NULL);

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS

	return gdk_pixbuf_get_from_surface (surface,
	                                    0, 0,
	                                    cairo_image_surface_get_width (surface),
	                                    cairo_image_surface_get_height (surface));
	G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * pps_document_misc_texture_from_surface:
 * @surface: a #cairo_surface_t
 *
 * Convert a cairo_surface_t to #GdkTexture object.
 *
 * Returns: (transfer full): The converted #GdkTexture
 */
GdkTexture *
pps_document_misc_texture_from_surface (cairo_surface_t *surface)
{
	GdkTexture *texture;
	GBytes *bytes;

	g_return_val_if_fail (surface != NULL, NULL);
	g_return_val_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE, NULL);
	g_return_val_if_fail (cairo_image_surface_get_width (surface) > 0, NULL);
	g_return_val_if_fail (cairo_image_surface_get_height (surface) > 0, NULL);

	bytes = g_bytes_new_with_free_func (cairo_image_surface_get_data (surface),
	                                    cairo_image_surface_get_height (surface) * cairo_image_surface_get_stride (surface),
	                                    (GDestroyNotify) cairo_surface_destroy,
	                                    cairo_surface_reference (surface));

	texture = gdk_memory_texture_new (cairo_image_surface_get_width (surface),
	                                  cairo_image_surface_get_height (surface),
	                                  GDK_MEMORY_DEFAULT,
	                                  bytes,
	                                  cairo_image_surface_get_stride (surface));

	g_bytes_unref (bytes);

	return texture;
}

cairo_surface_t *
pps_document_misc_surface_rotate_and_scale (cairo_surface_t *surface,
                                            gint dest_width,
                                            gint dest_height,
                                            gint dest_rotation)
{
	cairo_surface_t *new_surface;
	cairo_t *cr;
	gint width, height;
	gint new_width = dest_width;
	gint new_height = dest_height;

	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);

	if (dest_width == width &&
	    dest_height == height &&
	    dest_rotation == 0) {
		return cairo_surface_reference (surface);
	}

	if (dest_rotation == 90 || dest_rotation == 270) {
		new_width = dest_height;
		new_height = dest_width;
	}

	new_surface = cairo_surface_create_similar (surface,
	                                            cairo_surface_get_content (surface),
	                                            new_width, new_height);

	cr = cairo_create (new_surface);
	switch (dest_rotation) {
	case 90:
		cairo_translate (cr, new_width, 0);
		break;
	case 180:
		cairo_translate (cr, new_width, new_height);
		break;
	case 270:
		cairo_translate (cr, 0, new_height);
		break;
	default:
		cairo_translate (cr, 0, 0);
	}
	cairo_rotate (cr, dest_rotation * G_PI / 180.0);

	if (dest_width != width || dest_height != height) {
		cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_BILINEAR);
		cairo_scale (cr,
		             (gdouble) dest_width / width,
		             (gdouble) dest_height / height);
	}

	cairo_set_source_surface (cr, surface, 0, 0);
	cairo_paint (cr);
	cairo_destroy (cr);

	return new_surface;
}

/**
 * pps_document_misc_get_widget_dpi:
 * @widget: a #GtkWidget
 *
 * Returns sensible guess for DPI of monitor on which given widget has been
 * realized. If HiDPI display, use 192, else 96.
 * Returns 96 as fallback value.
 *
 * Returns: DPI as gdouble
 */
gdouble
pps_document_misc_get_widget_dpi (GtkWidget *widget)
{
	GdkDisplay *display = gtk_widget_get_display (widget);
	GtkNative *native = gtk_widget_get_native (widget);
	GdkSurface *surface = NULL;
	GdkMonitor *monitor = NULL;
	gboolean is_landscape;
	GdkRectangle geometry;

	if (native != NULL)
		surface = gtk_native_get_surface (native);

	if (surface != NULL) {
		monitor = gdk_display_get_monitor_at_surface (display, surface);
	}

	/* The only safe assumption you can make, on Unix-like/X11 and
	 * Linux/Wayland, is to always set the DPI to 96, regardless of
	 * physical/logical resolution, because that's the only safe
	 * guarantee we can make.
	 * https://gitlab.gnome.org/GNOME/gtk/-/issues/3115#note_904622 */
	if (monitor == NULL)
		return 96;

	gdk_monitor_get_geometry (monitor, &geometry);
	is_landscape = geometry.width > geometry.height;

	/* DPI is 192 if height ≥ 1080 and the orientation is not portrait,
	 * which is, incidentally, how GTK detects HiDPI displays and set a
	 * scaling factor for the logical output
	 * https://gitlab.gnome.org/GNOME/gtk/-/issues/3115#note_904622 */
	if (is_landscape && geometry.height >= 1080)
		return 192;
	else
		return 96;
}

/**
 * pps_document_misc_format_datetime:
 * @dt: a #GDateTime
 *
 * Determine the preferred date and time representation for the current locale
 * for @dt.
 *
 * Returns: (transfer full): a new allocated string or NULL in the case
 * that there was an error (such as a format specifier not being supported
 * in the current locale). The string should be freed with g_free().
 */
gchar *
pps_document_misc_format_datetime (GDateTime *dt)
{
	return g_date_time_format (dt, "%c");
}

/**
 * pps_document_misc_get_pointer_position:
 * @widget: a #GtkWidget
 * @x: (out): the pointer's "x" position, or -1 if the position is not
 *   available
 * @y: (out): the pointer's "y" position, or -1 if the position is not
 *   available
 *
 * Get the pointer's x and y position relative to @widget.
 */
gboolean
pps_document_misc_get_pointer_position (GtkWidget *widget,
                                        gint *x,
                                        gint *y)
{
	gdouble dx, dy;
	GdkSeat *seat;
	GtkNative *native;
	GdkDevice *device_pointer;
	GdkSurface *surface;
	graphene_point_t point;

	if (x)
		*x = -1;
	if (y)
		*y = -1;

	if (!gtk_widget_get_realized (widget))
		return FALSE;

	seat = gdk_display_get_default_seat (gtk_widget_get_display (widget));

	device_pointer = gdk_seat_get_pointer (seat);
	native = gtk_widget_get_native (widget);

	if (!native)
		return FALSE;

	surface = gtk_native_get_surface (native);
	if (!surface)
		return FALSE;

	if (!gdk_surface_get_device_position (surface,
	                                      device_pointer,
	                                      &dx, &dy, NULL))
		return FALSE;

	if (x)
		*x = dx;
	if (y)
		*y = dy;

	if (!gtk_widget_compute_point (widget, GTK_WIDGET (native),
	                               &GRAPHENE_POINT_INIT (0, 0), &point))
		g_warn_if_reached ();

	if (x)
		*x -= point.x;
	if (y)
		*y -= point.y;

	gtk_native_get_surface_transform (native, &dx, &dy);

	if (x)
		*x -= dx;
	if (y)
		*y -= dy;

	return TRUE;
}
