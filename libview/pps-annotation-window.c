/* pps-annotation-window.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2007 Iñigo Martinez <inigomartinez@gmail.com>
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

#include "config.h"

#include <string.h>
#include <math.h>
#include <adwaita.h>

#include <pps-document-annotations.h>

#include "pps-annotation-window.h"
#include "pps-color-contrast.h"
#include "pps-view-marshal.h"
#include "pps-document-misc.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

enum {
	PROP_0,
	PROP_ANNOTATION,
	PROP_PARENT,
	PROP_DOCUMENT,
};

struct _PpsAnnotationWindow {
	GtkWindow     base_instance;

	PpsAnnotation *annotation;
	GtkWindow     *parent;
	PpsDocument   *document;

	GtkWidget    *headerbar;
	GtkWidget    *title_label;
	GtkWidget    *text_view;

	gboolean      is_open;
	PpsRectangle   rect;
};

struct _PpsAnnotationWindowClass {
	GtkWindowClass base_class;
};

G_DEFINE_TYPE (PpsAnnotationWindow, pps_annotation_window, GTK_TYPE_WINDOW)

static void
pps_annotation_window_sync_contents (PpsAnnotationWindow *window)
{
	gchar         *contents;
	GtkTextIter    start, end;
	GtkTextBuffer *buffer;
	PpsAnnotation  *annot = window->annotation;

	if (!window->annotation)
		return;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->text_view));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	contents = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	if (pps_annotation_set_contents (annot, contents)) {
		pps_document_doc_mutex_lock (window->document);
		pps_document_annotations_save_annotation (PPS_DOCUMENT_ANNOTATIONS (window->document),
							  annot,
							  PPS_ANNOTATIONS_SAVE_CONTENTS);
		pps_document_doc_mutex_unlock (window->document);
	}

	g_free (contents);
}

static void
pps_annotation_window_set_color (PpsAnnotationWindow *window,
				GdkRGBA            *color)
{
	GtkCssProvider     *css_provider = gtk_css_provider_new ();
	GtkStyleContext    *context = NULL;
	g_autofree char    *rgba_str = gdk_rgba_to_string (color);
	g_autofree char    *css_data = NULL;
	g_autoptr (GdkRGBA) icon_color = pps_color_contrast_get_best_foreground_color (color);
	g_autofree char    *icon_color_str = gdk_rgba_to_string (icon_color);
	css_data = g_strdup_printf ("window { background-color: %s ; }", rgba_str);
	gtk_css_provider_load_from_string (css_provider, css_data);
	context = gtk_widget_get_style_context (GTK_WIDGET (window));
	gtk_style_context_add_provider (context,
					GTK_STYLE_PROVIDER (css_provider),
					GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_free (css_data);

	css_data = g_strdup_printf ("headerbar { background-color: %s ; color: %s; }",
				    rgba_str, icon_color_str);
	css_provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_string (css_provider, css_data);
	context = gtk_widget_get_style_context (GTK_WIDGET (window->headerbar));
	gtk_style_context_add_provider (context,
					GTK_STYLE_PROVIDER (css_provider),
					GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static void
pps_annotation_window_label_changed (PpsAnnotationMarkup *annot,
				    GParamSpec         *pspec,
				    PpsAnnotationWindow *window)
{
	const gchar *label = pps_annotation_markup_get_label (annot);

	gtk_label_set_text (GTK_LABEL (window->title_label), label);
}

static void
pps_annotation_window_color_changed (PpsAnnotation       *annot,
				    GParamSpec         *pspec,
				    PpsAnnotationWindow *window)
{
	GdkRGBA rgba;

	pps_annotation_get_rgba (annot, &rgba);
	pps_annotation_window_set_color (window, &rgba);
}

static void
pps_annotation_window_set_property (GObject      *object,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
	PpsAnnotationWindow *window = PPS_ANNOTATION_WINDOW (object);

	switch (prop_id) {
	case PROP_ANNOTATION:
		window->annotation = g_value_dup_object (value);
		break;
	case PROP_PARENT:
		window->parent = g_value_get_object (value);
		break;
	case PROP_DOCUMENT:
		window->document = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotation_window_init (PpsAnnotationWindow *window)
{
	GtkWidget *vbox;
	GtkWidget *swindow;

	gtk_widget_set_can_focus (GTK_WIDGET (window), TRUE);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	/* Title bar */
	window->headerbar = adw_header_bar_new ();
	gtk_window_set_titlebar (GTK_WINDOW (window), window->headerbar);

	window->title_label = gtk_label_new (NULL);
	gtk_widget_set_halign (window->title_label, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (window->title_label, TRUE);
	adw_header_bar_set_title_widget (ADW_HEADER_BAR (window->headerbar),
					 window->title_label);

	/* Contents */
	swindow = gtk_scrolled_window_new ();
	window->text_view = gtk_text_view_new ();

	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (window->text_view), TRUE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (window->text_view), GTK_WRAP_WORD);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (swindow), window->text_view);
	gtk_widget_set_valign (swindow, GTK_ALIGN_FILL);
	gtk_widget_set_vexpand (swindow, TRUE);
	gtk_window_set_focus (GTK_WINDOW (window), window->text_view);
	g_signal_connect_swapped (window->text_view, "notify::has-focus",
				  G_CALLBACK (pps_annotation_window_sync_contents),
				  window);

	gtk_box_append (GTK_BOX (vbox), swindow);

	gtk_window_set_child (GTK_WINDOW (window), vbox);

#ifdef GDK_WINDOWING_X11
	{
		GtkNative *native = gtk_widget_get_native (GTK_WIDGET (window));
		GdkSurface *surface = gtk_native_get_surface (native);

		if (GDK_IS_X11_SURFACE (surface)) {
			gdk_x11_surface_set_skip_taskbar_hint (GDK_X11_SURFACE (surface), TRUE);
			gdk_x11_surface_set_skip_pager_hint (GDK_X11_SURFACE (surface), TRUE);
		}
	}
#endif

	gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
	gtk_window_set_hide_on_close (GTK_WINDOW (window), TRUE);
	gtk_widget_add_css_class (GTK_WIDGET (window), "pps-annotation-window");
}

static GObject *
pps_annotation_window_constructor (GType                  type,
				  guint                  n_construct_properties,
				  GObjectConstructParam *construct_params)
{
	GObject            *object;
	PpsAnnotationWindow *window;
	PpsAnnotation       *annot;
	PpsAnnotationMarkup *markup;
	const gchar        *contents;
	const gchar        *label;
	GdkRGBA             color;
	PpsRectangle        *rect;
	gdouble             scale;

	object = G_OBJECT_CLASS (pps_annotation_window_parent_class)->constructor (type,
										  n_construct_properties,
										  construct_params);
	window = PPS_ANNOTATION_WINDOW (object);
	annot = window->annotation;
	markup = PPS_ANNOTATION_MARKUP (annot);

	gtk_window_set_transient_for (GTK_WINDOW (window), window->parent);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (window), FALSE);

	label = pps_annotation_markup_get_label (markup);
	window->is_open = pps_annotation_markup_get_popup_is_open (markup);
	pps_annotation_markup_get_rectangle (markup, &window->rect);

	rect = &window->rect;

	/* Rectangle is at doc resolution (72.0) */
	scale = pps_document_misc_get_widget_dpi (GTK_WIDGET (window)) / 72.0;
	gtk_window_set_default_size (GTK_WINDOW (window),
				     (gint)((rect->x2 - rect->x1) * scale),
				     (gint)((rect->y2 - rect->y1) * scale));

	pps_annotation_get_rgba (annot, &color);
	pps_annotation_window_set_color (window, &color);

	gtk_widget_set_name (GTK_WIDGET (window), pps_annotation_get_name (annot));
	gtk_label_set_text (GTK_LABEL (window->title_label), label);

	contents = pps_annotation_get_contents (annot);
	if (contents) {
		GtkTextBuffer *buffer;

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->text_view));
		gtk_text_buffer_set_text (buffer, contents, -1);
	}

	g_signal_connect (annot, "notify::label",
			  G_CALLBACK (pps_annotation_window_label_changed),
			  window);
	g_signal_connect (annot, "notify::rgba",
			  G_CALLBACK (pps_annotation_window_color_changed),
			  window);

	return object;
}

static gboolean
pps_annotation_window_escape_pressed (GtkWidget	*widget,
				     GVariant	*args,
				     gpointer	user_data)
{
	gtk_window_close (GTK_WINDOW (widget));
	return TRUE;
}

static void
pps_annotation_window_dispose (GObject *object)
{
	PpsAnnotationWindow *window = PPS_ANNOTATION_WINDOW (object);

	g_clear_object (&window->document);

	G_OBJECT_CLASS (pps_annotation_window_parent_class)->dispose (object);
}

static gboolean
pps_annotation_window_close_request (GtkWindow *window)
{
	pps_annotation_window_sync_contents (PPS_ANNOTATION_WINDOW (window));

	return GTK_WINDOW_CLASS (pps_annotation_window_parent_class)->close_request (window);
}

static void
pps_annotation_window_class_init (PpsAnnotationWindowClass *klass)
{
	GObjectClass   *g_object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *gtk_widget_class = GTK_WIDGET_CLASS (klass);
	GtkWindowClass *gtk_window_class = GTK_WINDOW_CLASS (klass);

	g_object_class->constructor = pps_annotation_window_constructor;
	g_object_class->set_property = pps_annotation_window_set_property;
	g_object_class->dispose = pps_annotation_window_dispose;
	gtk_window_class->close_request = pps_annotation_window_close_request;

	gtk_widget_class_add_binding (gtk_widget_class, GDK_KEY_Escape, 0,
				      pps_annotation_window_escape_pressed, NULL);

	g_object_class_install_property (g_object_class,
					 PROP_ANNOTATION,
					 g_param_spec_object ("annotation",
							      "Annotation",
							      "The annotation associated to the window",
							      PPS_TYPE_ANNOTATION_MARKUP,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
					 PROP_PARENT,
					 g_param_spec_object ("parent",
							      "Parent",
							      "The parent window",
							      GTK_TYPE_WINDOW,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
					 PROP_DOCUMENT,
					 g_param_spec_object ("document",
							      NULL, NULL,
							      PPS_TYPE_DOCUMENT,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));
}

/* Public methods */
GtkWidget *
pps_annotation_window_new (PpsAnnotation *annot,
			   GtkWindow     *parent,
			   PpsDocument   *document)
{
	GtkWidget *window;

	g_return_val_if_fail (PPS_IS_ANNOTATION_MARKUP (annot), NULL);
	g_return_val_if_fail (GTK_IS_WINDOW (parent), NULL);

	window = g_object_new (PPS_TYPE_ANNOTATION_WINDOW,
			       "annotation", annot,
			       "parent", parent,
			       "document", document,
			       NULL);
	return window;
}

PpsAnnotation *
pps_annotation_window_get_annotation (PpsAnnotationWindow *window)
{
	g_return_val_if_fail (PPS_IS_ANNOTATION_WINDOW (window), NULL);

	return window->annotation;
}

gboolean
pps_annotation_window_is_open (PpsAnnotationWindow *window)
{
	g_return_val_if_fail (PPS_IS_ANNOTATION_WINDOW (window), FALSE);

	return window->is_open;
}

void
pps_annotation_window_get_rectangle (PpsAnnotationWindow *window,
				    PpsRectangle        *rect)
{
	g_return_if_fail (PPS_IS_ANNOTATION_WINDOW (window));
	g_return_if_fail (rect != NULL);

	*rect = window->rect;
}

void
pps_annotation_window_set_rectangle (PpsAnnotationWindow *window,
				    const PpsRectangle  *rect)
{
	g_return_if_fail (PPS_IS_ANNOTATION_WINDOW (window));
	g_return_if_fail (rect != NULL);

	window->rect = *rect;
}

void
pps_annotation_window_set_enable_spellchecking (PpsAnnotationWindow *window,
                                               gboolean enable_spellchecking)
{
        g_return_if_fail (PPS_IS_ANNOTATION_WINDOW (window));
}

gboolean
pps_annotation_window_get_enable_spellchecking (PpsAnnotationWindow *window)
{
        g_return_val_if_fail (PPS_IS_ANNOTATION_WINDOW (window), FALSE);
        return FALSE;
}
