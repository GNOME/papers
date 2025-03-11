// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Implements hyperlink functionality for Djvu files.
 * Copyright (C) 2006 Pauli Virtanen <pav@iki.fi>
 */

#include "djvu-links.h"
#include "djvu-document-private.h"
#include "djvu-document.h"
#include "pps-document-links.h"
#include "pps-mapping-list.h"
#include "pps-outlines.h"
#include <config.h>
#include <glib.h>
#include <libdjvu/miniexp.h>
#include <string.h>

static gboolean
number_from_miniexp (miniexp_t sexp, int *number)
{
	if (miniexp_numberp (sexp)) {
		*number = miniexp_to_int (sexp);
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean
string_from_miniexp (miniexp_t sexp, const char **str)
{
	if (miniexp_stringp (sexp)) {
		*str = miniexp_to_str (sexp);
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean
number_from_string_10 (const gchar *str, guint64 *number)
{
	gchar *end_ptr;

	*number = g_ascii_strtoull (str, &end_ptr, 10);
	if (*end_ptr == '\0') {
		return TRUE;
	} else {
		return FALSE;
	}
}

static guint64
get_djvu_link_page (const DjvuDocument *djvu_document, const gchar *link_name, int base_page)
{
	guint64 page_num = 0;

	/* #pagenum, #+pageoffset, #-pageoffset */
	if (g_str_has_prefix (link_name, "#")) {
		if (g_str_has_suffix (link_name, ".djvu")) {
			/* File identifiers */
			gpointer page = NULL;

			if (g_hash_table_lookup_extended (djvu_document->file_ids, link_name + 1, NULL, &page)) {
				return GPOINTER_TO_INT (page);
			}
		} else if (base_page > 0 && g_str_has_prefix (link_name + 1, "+")) {
			if (number_from_string_10 (link_name + 2, &page_num)) {
				return base_page + page_num;
			}
		} else if (base_page > 0 && g_str_has_prefix (link_name + 1, "-")) {
			if (number_from_string_10 (link_name + 2, &page_num)) {
				return base_page - page_num;
			}
		} else {
			if (number_from_string_10 (link_name + 1, &page_num)) {
				return page_num - 1;
			}
		}
	} else {
		/* FIXME: should we handle this case */
	}

	return page_num;
}

static PpsLinkDest *
get_djvu_link_dest (const DjvuDocument *djvu_document, const gchar *link_name, int base_page)
{
	/* #+pagenum #-pagenum #file_id.djvu */
	if (g_str_has_prefix (link_name, "#")) {
		if (g_str_has_suffix (link_name, ".djvu") ||
		    (base_page > 0 && g_str_has_prefix (link_name + 1, "+")) ||
		    (base_page > 0 && g_str_has_prefix (link_name + 1, "-"))) {
			return pps_link_dest_new_page (get_djvu_link_page (djvu_document, link_name, base_page));
		} else {
			/* #pagenum #page_label: the djvu spec is not clear on whether #pagenum represents
			 * a link to a page number or to a page label. Here we mimic djview,
			 * and always treat #pagenum as a link to a page label */
			return pps_link_dest_new_page_label (link_name + 1);
		}
	}

	return NULL;
}

static PpsLinkAction *
get_djvu_link_action (const DjvuDocument *djvu_document, const gchar *link_name, int base_page)
{
	PpsLinkDest *pps_dest = NULL;
	PpsLinkAction *pps_action = NULL;

	/* File component identifiers are handled by get_djvu_link_dest */

	pps_dest = get_djvu_link_dest (djvu_document, link_name, base_page);
	if (pps_dest) {
		pps_action = pps_link_action_new_dest (pps_dest);
		g_object_unref (pps_dest);
	} else if (strstr (link_name, "://") != NULL) {
		/* It's probably an URI */
		pps_action = pps_link_action_new_external_uri (link_name);
	}

	return pps_action;
}

static gchar *
str_to_utf8 (const gchar *text)
{
	static const gchar *encodings_to_try[2];
	static gint n_encodings_to_try = 0;
	gchar *utf8_text = NULL;
	gint i;

	if (n_encodings_to_try == 0) {
		const gchar *charset;
		gboolean charset_is_utf8;

		charset_is_utf8 = g_get_charset (&charset);
		if (!charset_is_utf8) {
			encodings_to_try[n_encodings_to_try++] = charset;
		}

		if (g_ascii_strcasecmp (charset, "ISO-8859-1") != 0) {
			encodings_to_try[n_encodings_to_try++] = "ISO-8859-1";
		}
	}

	for (i = 0; i < n_encodings_to_try; i++) {
		utf8_text = g_convert (text, -1, "UTF-8",
		                       encodings_to_try[i],
		                       NULL, NULL, NULL);
		if (utf8_text)
			break;
	}

	return utf8_text;
}

/**
 * Builds the index GListStore from DjVu s-expr
 *
 * (bookmarks
 *   ("title1" "dest1"
 *     ("title12" "dest12"
 *       ... )
 *     ... )
 *   ("title2" "dest2"
 *     ... )
 *   ... )
 */
static void
build_tree (const DjvuDocument *djvu_document,
            GListStore *model,
            miniexp_t iter)
{
	const char *title, *link_dest;
	g_autofree gchar *title_markup = NULL;
	g_autofree gchar *utf8_title = NULL;

	g_autoptr (PpsLinkAction) pps_action = NULL;
	g_autoptr (PpsLink) pps_link = NULL;
	g_autoptr (PpsOutlines) outlines = NULL;

	if (miniexp_car (iter) == miniexp_symbol ("bookmarks")) {
		/* The (bookmarks) cons */
		iter = miniexp_cdr (iter);
	} else if (miniexp_length (iter) >= 2) {
		/* An entry */
		if (!string_from_miniexp (miniexp_car (iter), &title))
			goto unknown_entry;
		if (!string_from_miniexp (miniexp_cadr (iter), &link_dest))
			goto unknown_entry;

		if (!g_utf8_validate (title, -1, NULL)) {
			utf8_title = str_to_utf8 (title);
			title_markup = g_markup_escape_text (utf8_title, -1);
		} else {
			title_markup = g_markup_escape_text (title, -1);
		}

		pps_action = get_djvu_link_action (djvu_document, link_dest, -1);

		if (pps_action) {
			pps_link = pps_link_new (utf8_title ? utf8_title : title, pps_action);
		}

		outlines = g_object_new (PPS_TYPE_OUTLINES, "markup", title_markup, "expand", FALSE, "link", pps_link, NULL);
		g_list_store_append (model, outlines);

		iter = miniexp_cddr (iter);
	} else {
		goto unknown_entry;
	}

	if (outlines && iter != miniexp_nil) {
		model = g_list_store_new (PPS_TYPE_OUTLINES);
		g_object_set (outlines, "children", model, NULL);
	}

	for (; iter != miniexp_nil; iter = miniexp_cdr (iter)) {
		build_tree (djvu_document, model, miniexp_car (iter));
	}

	return;

unknown_entry:
	g_warning ("DjvuLibre error: Unknown entry in bookmarks");
	return;
}

static gboolean
get_djvu_hyperlink_area (ddjvu_pageinfo_t *page_info,
                         miniexp_t sexp,
                         PpsMapping *pps_link_mapping)
{
	miniexp_t iter;

	iter = sexp;

	if ((miniexp_car (iter) == miniexp_symbol ("rect") || miniexp_car (iter) == miniexp_symbol ("oval")) && miniexp_length (iter) == 5) {
		/* FIXME: get bounding box for (oval) since Papers doesn't support shaped links */
		int minx, miny, width, height;

		iter = miniexp_cdr (iter);
		if (!number_from_miniexp (miniexp_car (iter), &minx))
			goto unknown_link;
		iter = miniexp_cdr (iter);
		if (!number_from_miniexp (miniexp_car (iter), &miny))
			goto unknown_link;
		iter = miniexp_cdr (iter);
		if (!number_from_miniexp (miniexp_car (iter), &width))
			goto unknown_link;
		iter = miniexp_cdr (iter);
		if (!number_from_miniexp (miniexp_car (iter), &height))
			goto unknown_link;

		pps_link_mapping->area.x1 = minx;
		pps_link_mapping->area.x2 = (minx + width);
		pps_link_mapping->area.y1 = (page_info->height - (miny + height));
		pps_link_mapping->area.y2 = (page_info->height - miny);
	} else if (miniexp_car (iter) == miniexp_symbol ("poly") && miniexp_length (iter) >= 5 && miniexp_length (iter) % 2 == 1) {

		/* FIXME: get bounding box since Papers doesn't support shaped links */
		int minx = G_MAXINT, miny = G_MAXINT;
		int maxx = G_MININT, maxy = G_MININT;

		iter = miniexp_cdr (iter);
		while (iter != miniexp_nil) {
			int x, y;

			if (!number_from_miniexp (miniexp_car (iter), &x))
				goto unknown_link;
			iter = miniexp_cdr (iter);
			if (!number_from_miniexp (miniexp_car (iter), &y))
				goto unknown_link;
			iter = miniexp_cdr (iter);

			minx = MIN (minx, x);
			miny = MIN (miny, y);
			maxx = MAX (maxx, x);
			maxy = MAX (maxy, y);
		}

		pps_link_mapping->area.x1 = minx;
		pps_link_mapping->area.x2 = maxx;
		pps_link_mapping->area.y1 = (page_info->height - maxy);
		pps_link_mapping->area.y2 = (page_info->height - miny);
	} else {
		/* unknown */
		goto unknown_link;
	}

	return TRUE;

unknown_link:
	g_warning ("DjvuLibre error: Unknown hyperlink area %s", miniexp_to_name (miniexp_car (sexp)));
	return FALSE;
}

static PpsMapping *
get_djvu_hyperlink_mapping (DjvuDocument *djvu_document,
                            int page,
                            ddjvu_pageinfo_t *page_info,
                            miniexp_t sexp)
{
	PpsMapping *pps_link_mapping = NULL;
	PpsLinkAction *pps_action = NULL;
	miniexp_t iter;
	const char *url, *url_target, *comment;

	pps_link_mapping = g_new (PpsMapping, 1);

	iter = sexp;

	if (miniexp_car (iter) != miniexp_symbol ("maparea"))
		goto unknown_mapping;

	iter = miniexp_cdr (iter);

	if (miniexp_caar (iter) == miniexp_symbol ("url")) {
		if (!string_from_miniexp (miniexp_cadr (miniexp_car (iter)), &url))
			goto unknown_mapping;
		if (!string_from_miniexp (miniexp_caddr (miniexp_car (iter)), &url_target))
			goto unknown_mapping;
	} else {
		if (!string_from_miniexp (miniexp_car (iter), &url))
			goto unknown_mapping;
		url_target = NULL;
	}

	iter = miniexp_cdr (iter);
	if (!string_from_miniexp (miniexp_car (iter), &comment))
		goto unknown_mapping;

	iter = miniexp_cdr (iter);
	if (!get_djvu_hyperlink_area (page_info, miniexp_car (iter), pps_link_mapping))
		goto unknown_mapping;

	iter = miniexp_cdr (iter);
	/* FIXME: DjVu hyperlink attributes are ignored */

	pps_action = get_djvu_link_action (djvu_document, url, page);
	if (!pps_action)
		goto unknown_mapping;

	pps_link_mapping->data = pps_link_new (comment, pps_action);
	g_object_unref (pps_action);

	return pps_link_mapping;

unknown_mapping:
	if (pps_link_mapping)
		g_free (pps_link_mapping);
	g_warning ("DjvuLibre error: Unknown hyperlink %s", miniexp_to_name (miniexp_car (sexp)));
	return NULL;
}

gboolean
djvu_links_has_document_links (PpsDocumentLinks *document_links)
{
	DjvuDocument *djvu_document = DJVU_DOCUMENT (document_links);
	miniexp_t outline;
	gboolean has_links;

	g_rw_lock_reader_lock (&djvu_document->rwlock);

	while ((outline = ddjvu_document_get_outline (djvu_document->d_document)) == miniexp_dummy)
		djvu_handle_events (djvu_document, TRUE, NULL);

	has_links = outline != NULL;
	if (outline) {
		ddjvu_miniexp_release (djvu_document->d_document, outline);
	}

	g_rw_lock_reader_unlock (&djvu_document->rwlock);

	return has_links;
}

PpsMappingList *
djvu_links_get_links (PpsDocumentLinks *document_links,
                      gint page,
                      double scale_factor)
{
	DjvuDocument *djvu_document = DJVU_DOCUMENT (document_links);
	GList *retval = NULL;
	miniexp_t page_annotations = miniexp_nil;
	miniexp_t *hyperlinks = NULL, *iter = NULL;
	PpsMapping *pps_link_mapping;
	ddjvu_pageinfo_t page_info;

	g_rw_lock_reader_lock (&djvu_document->rwlock);

	while ((page_annotations = ddjvu_document_get_pageanno (djvu_document->d_document, page)) == miniexp_dummy)
		djvu_handle_events (djvu_document, TRUE, NULL);

	while (ddjvu_document_get_pageinfo (djvu_document->d_document, page, &page_info) < DDJVU_JOB_OK)
		djvu_handle_events (djvu_document, TRUE, NULL);

	if (page_annotations) {
		hyperlinks = ddjvu_anno_get_hyperlinks (page_annotations);
		if (hyperlinks) {
			for (iter = hyperlinks; *iter; ++iter) {
				pps_link_mapping = get_djvu_hyperlink_mapping (djvu_document, page, &page_info, *iter);
				if (pps_link_mapping) {
					pps_link_mapping->area.x1 *= scale_factor;
					pps_link_mapping->area.x2 *= scale_factor;
					pps_link_mapping->area.y1 *= scale_factor;
					pps_link_mapping->area.y2 *= scale_factor;
					retval = g_list_prepend (retval, pps_link_mapping);
				}
			}
			free (hyperlinks);
		}
		ddjvu_miniexp_release (djvu_document->d_document, page_annotations);
	}

	g_rw_lock_reader_unlock (&djvu_document->rwlock);

	return pps_mapping_list_new (page, retval, (GDestroyNotify) g_object_unref);
}

PpsLinkDest *
djvu_links_find_link_dest (PpsDocumentLinks *document_links,
                           const gchar *link_name)
{
	DjvuDocument *djvu_document = DJVU_DOCUMENT (document_links);
	PpsLinkDest *pps_dest = NULL;

	g_rw_lock_reader_lock (&djvu_document->rwlock);

	pps_dest = get_djvu_link_dest (djvu_document, link_name, -1);

	if (!pps_dest) {
		g_warning ("DjvuLibre error: unknown link destination %s", link_name);
	}

	g_rw_lock_reader_unlock (&djvu_document->rwlock);

	return pps_dest;
}

gint
djvu_links_find_link_page (PpsDocumentLinks *document_links,
                           const gchar *link_name)
{
	DjvuDocument *djvu_document = DJVU_DOCUMENT (document_links);
	gint page;

	g_rw_lock_reader_lock (&djvu_document->rwlock);

	page = get_djvu_link_page (djvu_document, link_name, -1);

	if (page == -1) {
		g_warning ("DjvuLibre error: unknown link destination %s", link_name);
	}

	g_rw_lock_reader_unlock (&djvu_document->rwlock);

	return page;
}

GListModel *
djvu_links_get_links_model (PpsDocumentLinks *document_links)
{
	DjvuDocument *djvu_document = DJVU_DOCUMENT (document_links);
	GListStore *model = NULL;
	miniexp_t outline = miniexp_nil;

	g_rw_lock_reader_lock (&djvu_document->rwlock);

	while ((outline = ddjvu_document_get_outline (djvu_document->d_document)) == miniexp_dummy)
		djvu_handle_events (djvu_document, TRUE, NULL);

	if (outline) {
		model = g_list_store_new (PPS_TYPE_OUTLINES);
		build_tree (djvu_document, model, outline);

		ddjvu_miniexp_release (djvu_document->d_document, outline);

		g_rw_lock_reader_unlock (&djvu_document->rwlock);
		return G_LIST_MODEL (model);
	}

	g_rw_lock_reader_unlock (&djvu_document->rwlock);
	return NULL;
}
