// SPDX-FileCopyrightText: 2005 Fernando Herrera <fherrera@onirica.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#include <config.h>

#include <papers-document.h>

#include <gio/gio.h>

#include <locale.h>
#include <stdlib.h>
#include <string.h>

#define THUMBNAIL_SIZE 128
#define DEFAULT_SLEEP_TIME (15 * G_USEC_PER_SEC) /* 15 seconds */

static gboolean finished = TRUE;

static gint size = THUMBNAIL_SIZE;
static gboolean time_limit = TRUE;
static const gchar **file_arguments;

static const GOptionEntry goption_options[] = {
	{ "size", 's', 0, G_OPTION_ARG_INT, &size, NULL, "SIZE" },
	{ "no-limit", 'l', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &time_limit, "Don't limit the thumbnailing time to 15 seconds", NULL },
	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &file_arguments, NULL, "<input> <output>" },
	{ NULL }
};

/* Time monitor: copied from totem */
G_GNUC_NORETURN static gpointer
time_monitor (gpointer data)
{
	const gchar *app_name;

	g_usleep (DEFAULT_SLEEP_TIME);

	if (finished)
		g_thread_exit (NULL);

	app_name = g_get_application_name ();
	if (app_name == NULL)
		app_name = g_get_prgname ();
	g_printerr ("%s couldn't process file: '%s'\n"
	            "Reason: Took too much time to process.\n",
	            app_name,
	            (const char *) data);

	exit (0);
}

static void
time_monitor_start (const char *input)
{
	finished = FALSE;

	g_thread_new ("ThumbnailerTimer", time_monitor, (gpointer) input);
}

static void
time_monitor_stop (void)
{
	finished = TRUE;
}

static void
delete_temp_file (gpointer data, GObject *object)
{
	GFile *file = G_FILE (data);

	pps_tmp_file_unlink (file);
	g_object_unref (file);
}

static char *
get_target_uri (GFile *file)
{
	g_autoptr (GFileInfo) info = NULL;
	char *target;

	info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	if (info == NULL)
		return NULL;
	target = g_strdup (g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI));

	return target;
}

static char *
get_local_path (GFile *file)
{
	if (g_file_has_uri_scheme (file, "trash") != FALSE ||
	    g_file_has_uri_scheme (file, "recent") != FALSE) {
		return get_target_uri (file);
	}
	return g_file_get_path (file);
}

static PpsDocument *
papers_thumbnailer_get_document (GFile *file)
{
	g_autoptr (PpsDocument) document = NULL;
	g_autofree gchar *uri = NULL;
	g_autofree gchar *path = NULL;
	g_autoptr (GFile) tmp_file = NULL;
	g_autoptr (GError) error = NULL;

	path = get_local_path (file);

	if (!path) {
		g_autofree gchar *base_name = NULL;
		g_autofree gchar *template = NULL;

		base_name = g_file_get_basename (file);
		template = g_strdup_printf ("document.XXXXXX-%s", base_name);

		tmp_file = pps_mkstemp_file (template, &error);
		if (!tmp_file) {
			g_printerr ("Error loading remote document: %s\n", error->message);

			return NULL;
		}

		g_file_copy (file, tmp_file, G_FILE_COPY_OVERWRITE,
		             NULL, NULL, NULL, &error);
		if (error) {
			g_printerr ("Error loading remote document: %s\n", error->message);
			return NULL;
		}
		uri = g_file_get_uri (tmp_file);
	} else {
		uri = g_filename_to_uri (path, NULL, NULL);
	}

	document = pps_document_factory_get_document (uri, &error);
	if (document != NULL)
		pps_document_load_full (document, uri,
		                        PPS_DOCUMENT_LOAD_FLAG_NO_CACHE,
		                        &error);

	if (tmp_file) {
		if (document) {
			g_object_weak_ref (G_OBJECT (document),
			                   delete_temp_file,
			                   tmp_file);
		} else {
			pps_tmp_file_unlink (tmp_file);
		}
	}
	if (error) {
		if (g_error_matches (error, PPS_DOCUMENT_ERROR,
		                     PPS_DOCUMENT_ERROR_ENCRYPTED)) {
			/* FIXME: Create a thumb for cryp docs */
			return NULL;
		}
		g_printerr ("Error loading document: %s\n", error->message);
		return NULL;
	}

	return g_steal_pointer (&document);
}

static gboolean
papers_thumbnail_pngenc_get (PpsDocument *document, const char *thumbnail, int size)
{
	double width, height;
	g_autoptr (PpsRenderContext) rc = NULL;
	g_autoptr (GdkPixbuf) pixbuf = NULL;
	g_autoptr (PpsPage) page = pps_document_get_page (document, 0);

	PPS_DOCUMENT_GET_CLASS (document)->get_page_size (document, page, &width, &height);

	rc = pps_render_context_new (page, 0, size / MAX (height, width), PPS_RENDER_ANNOTS_ALL);
	pixbuf = pps_document_get_thumbnail (document, rc);

	if (pixbuf && gdk_pixbuf_save (pixbuf, thumbnail, "png", NULL, NULL))
		return TRUE;

	return FALSE;
}

static void
print_usage (GOptionContext *context)
{
	gchar *help;

	help = g_option_context_get_help (context, TRUE, NULL);
	g_print ("%s", help);
	g_free (help);
}

int
main (int argc, char *argv[])
{
	g_autoptr (PpsDocument) document = NULL;
	GOptionContext *context;
	const char *input;
	const char *output;
	g_autoptr (GFile) file = NULL;
	g_autoptr (GError) error = NULL;

	setlocale (LC_ALL, "");

	context = g_option_context_new ("- GNOME Document Thumbnailer");
	g_option_context_add_main_entries (context, goption_options, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("%s\n", error->message);
		print_usage (context);
		g_option_context_free (context);

		return -1;
	}

	input = file_arguments ? file_arguments[0] : NULL;
	output = input ? file_arguments[1] : NULL;
	if (!input || !output) {
		print_usage (context);
		g_option_context_free (context);

		return -1;
	}

	g_option_context_free (context);

	if (size < 1) {
		g_printerr ("Size cannot be smaller than 1 pixel\n");
		return -1;
	}

	input = file_arguments[0];
	output = file_arguments[1];

	if (!pps_init ())
		return -1;

	file = g_file_new_for_commandline_arg (input);
	document = papers_thumbnailer_get_document (file);

	if (!document) {
		pps_shutdown ();
		return -2;
	}

	if (time_limit)
		time_monitor_start (input);

	if (!papers_thumbnail_pngenc_get (document, output, size)) {
		pps_shutdown ();
		return -2;
	}

	time_monitor_stop ();
	pps_shutdown ();

	return 0;
}
