/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/*
 *  Copyright (C) 2004 Anders Carlsson <andersca@gnome.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config.h>

#include "pps-utils.h"

#include <string.h>
#include <glib/gi18n.h>

/* Simple function to output the contents of a region.  Used solely for testing
 * the region code.
 */
void
pps_print_region_contents (cairo_region_t *region)
{
	gint n_rectangles, i;

	if (region == NULL) {
		g_print ("<empty region>\n");
		return;
	}

	g_print ("<region %p>\n", region);
	n_rectangles = cairo_region_num_rectangles (region);
	for (i = 0; i < n_rectangles; i++) {
		GdkRectangle rect;

		cairo_region_get_rectangle (region, i, &rect);
		g_print ("\t(%d %d, %d %d) [%dx%d]\n",
			 rect.x,
			 rect.y,
			 rect.x + rect.width,
			 rect.y + rect.height,
			 rect.width,
			 rect.height);
	}
}

GdkPixbufFormat*
pps_gdk_pixbuf_format_by_extension (const gchar *uri)
{
	GSList *pixbuf_formats = NULL;
	GSList *iter;
	int i;

	pixbuf_formats = gdk_pixbuf_get_formats ();

	for (iter = pixbuf_formats; iter; iter = iter->next) {
		gchar **extension_list;
		GdkPixbufFormat *format = iter->data;

		if (gdk_pixbuf_format_is_disabled (format) ||
	    	    !gdk_pixbuf_format_is_writable (format))
		            continue;

	        extension_list = gdk_pixbuf_format_get_extensions (format);

		for (i = 0; extension_list[i] != 0; i++) {
			if (g_str_has_suffix (uri, extension_list[i])) {
			    	g_slist_free (pixbuf_formats);
				g_strfreev (extension_list);
				return format;
			}
		}
		g_strfreev (extension_list);
	}

	g_slist_free (pixbuf_formats);
	return NULL;
}

/*
 * Replace all occurrences of substr in str with repl
 *
 * @param str a string
 * @param substr some string to replace
 * @param repl a replacement string
 *
 * @return a newly allocated string with the substr replaced by repl; free with g_free
 */
gchar*
pps_str_replace (const char *str, const char *substr, const char *repl)
{
	GString		*gstr;
	const char	*cur;

	if (str == NULL || substr == NULL || repl == NULL)
		return NULL;

	gstr = g_string_sized_new (2 * strlen (str));

	for (cur = str; *cur; ++cur) {
		if (g_str_has_prefix (cur, substr)) {
			g_string_append (gstr, repl);
			cur += strlen (substr) - 1;
		} else
			g_string_append_c (gstr, *cur);
	}

	return g_string_free (gstr, FALSE);
}

void
pps_spinner_map_cb (GtkSpinner *spinner) {
	gtk_spinner_set_spinning(spinner, TRUE);
}

void
pps_spinner_unmap_cb (GtkSpinner *spinner) {
	gtk_spinner_set_spinning(spinner, FALSE);
}


/**
 * pps_spawn:
 * @uri: (nullable): The uri to be opened.
 * @dest: (nullable): The #PpsLinkDest of the document.
 * @mode: The run mode of the window.
 */
void
pps_spawn (const char	  *uri,
	   PpsLinkDest	   *dest,
	   PpsWindowRunMode mode)
{
	GString *cmd;
	gchar *path, *cmdline;
	GAppInfo *app;
	GError	*error = NULL;

	cmd = g_string_new (NULL);

	path = g_find_program_in_path ("papers");

	g_string_append_printf (cmd, " %s", path);
	g_free (path);

	/* Page label */
	if (dest) {
		switch (pps_link_dest_get_dest_type (dest)) {
		case PPS_LINK_DEST_TYPE_PAGE_LABEL:
			g_string_append_printf (cmd, " --page-label=%s",
						pps_link_dest_get_page_label (dest));
			break;
		case PPS_LINK_DEST_TYPE_PAGE:
		case PPS_LINK_DEST_TYPE_XYZ:
		case PPS_LINK_DEST_TYPE_FIT:
		case PPS_LINK_DEST_TYPE_FITH:
		case PPS_LINK_DEST_TYPE_FITV:
		case PPS_LINK_DEST_TYPE_FITR:
			g_string_append_printf (cmd, " --page-index=%d",
						pps_link_dest_get_page (dest) + 1);
			break;
		case PPS_LINK_DEST_TYPE_NAMED:
			g_string_append_printf (cmd, " --named-dest=%s",
						pps_link_dest_get_named_dest (dest));
			break;
		default:
			break;
		}
	}

	/* Mode */
	switch (mode) {
	case PPS_WINDOW_MODE_FULLSCREEN:
		g_string_append (cmd, " -f");
		break;
	case PPS_WINDOW_MODE_PRESENTATION:
		g_string_append (cmd, " -s");
		break;
	default:
		break;
	}

	cmdline = g_string_free (cmd, FALSE);
	app = g_app_info_create_from_commandline (cmdline, NULL, G_APP_INFO_CREATE_SUPPORTS_URIS, &error);

	if (app != NULL) {
		GList uri_list;
		GList *uris = NULL;
		GdkAppLaunchContext *ctx;

		ctx = gdk_display_get_app_launch_context (gdk_display_get_default ());

		/* Some URIs can be changed when passed through a GFile
		 * (for instance unsupported uris with strange formats like mailto:),
		 * so if you have a textual uri you want to pass in as argument,
		 * consider using g_app_info_launch_uris() instead.
		 * See https://bugzilla.gnome.org/show_bug.cgi?id=644604
		 */
		if (uri) {
			uri_list.data = (gchar *)uri;
			uri_list.prev = uri_list.next = NULL;
			uris = &uri_list;
		}
		g_app_info_launch_uris (app, uris, G_APP_LAUNCH_CONTEXT (ctx), &error);

		g_object_unref (app);
		g_object_unref (ctx);
	}

	if (error != NULL) {
		g_printerr ("Error launching papers %s: %s\n", uri, error->message);
		g_error_free (error);
	}

	g_free (cmdline);
}
