// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Declarations used throughout the djvu classes
 *
 * Copyright (C) 2006, Michael Hofmann <mh21@piware.de>
 */

#pragma once

#include "djvu-document.h"

#include <libdjvu/ddjvuapi.h>

struct _DjvuDocument {
	PpsDocument parent_instance;

	ddjvu_context_t *d_context;
	ddjvu_document_t *d_document;
	ddjvu_format_t *d_format;
	ddjvu_format_t *thumbs_format;

	gchar *uri;

	/* PS exporter */
	gchar *ps_filename;
	GString *opts;
	ddjvu_fileinfo_t *fileinfo_pages;
	gint n_pages;
	GHashTable *file_ids;

	GRWLock rwlock;
};

int djvu_document_get_n_pages (PpsDocument *document);
void djvu_handle_events (DjvuDocument *djvu_document,
                         int wait,
                         GError **error);
