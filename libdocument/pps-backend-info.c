// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#include <config.h>

#include "pps-backend-info.h"

#define PPS_BACKENDS_GROUP "Papers Backend"
#define PPS_BACKENDS_EXTENSION ".papers-backend"

void
_pps_backend_info_free (PpsBackendInfo *info)
{
	if (info == NULL)
		return;

	g_free (info->module_name);
	g_free (info->type_desc);
	g_strfreev (info->mime_types);
	g_free (info);
}

/**
 * _pps_backend_info_new_from_file:
 * @path: path to the backends file
 * @error: a location to store a #GError, or %NULL
 *
 * Loads backend information from @path.
 *
 * Returns: a new #PpsBackendInfo, or %NULL on error with @error filled in
 */
static PpsBackendInfo *
_pps_backend_info_new_from_file (const char *file,
                                 GError **error)
{
	PpsBackendInfo *info = NULL;
	g_autoptr (GKeyFile) backend_file = NULL;

	backend_file = g_key_file_new ();
	if (!g_key_file_load_from_file (backend_file, file, G_KEY_FILE_NONE, error))
		goto err;

	info = g_new0 (PpsBackendInfo, 1);

	info->module_name = g_key_file_get_string (backend_file, PPS_BACKENDS_GROUP,
	                                           "Module", error);
	if (!info->module_name)
		goto err;

	info->resident = g_key_file_get_boolean (backend_file, PPS_BACKENDS_GROUP,
	                                         "Resident", NULL);

	info->type_desc = g_key_file_get_locale_string (backend_file, PPS_BACKENDS_GROUP,
	                                                "TypeDescription", NULL, error);
	if (!info->type_desc)
		goto err;

	info->mime_types = g_key_file_get_string_list (backend_file, PPS_BACKENDS_GROUP,
	                                               "MimeType", NULL, error);
	if (!info->mime_types)
		goto err;

	return info;

err:
	if (info)
		_pps_backend_info_free (info);

	return NULL;
}

/*
 * _pps_backend_info_load_from_dir:
 * @path: a directory name
 *
 * Load all backend infos from @path.
 *
 * Returns: a newly allocated #GList containing newly allocated
 *   #PpsBackendInfo objects
 */
GList *
_pps_backend_info_load_from_dir (const char *path)
{
	GList *list = NULL;
	GDir *dir;
	const gchar *dirent;
	g_autoptr (GError) error = NULL;

	dir = g_dir_open (path, 0, &error);
	if (!dir) {
		g_warning ("%s", error->message);
		return FALSE;
	}

	while ((dirent = g_dir_read_name (dir))) {
		PpsBackendInfo *info;
		gchar *file;

		if (!g_str_has_suffix (dirent, PPS_BACKENDS_EXTENSION))
			continue;

		file = g_build_filename (path, dirent, NULL);
		info = _pps_backend_info_new_from_file (file, &error);
		if (error != NULL) {
			g_warning ("Failed to load backend info from '%s': %s\n",
			           file, error->message);
			g_clear_error (&error);
		}
		g_free (file);

		if (info == NULL)
			continue;

		list = g_list_prepend (list, info);
	}

	g_dir_close (dir);

	return list;
}
