/* pps-search-context.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2015 Igalia S.L.
 * Copyright (C) 2024 Markus Göllnitz  <camelcasenick@bewares.it>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "pps-jobs.h"
#include "pps-search-context.h"
#include <papers-view.h>

enum {
	PROP_0,

	PROP_DOCUMENT_MODEL,
	PROP_SEARCH_TERM,

	NUM_PROPERTIES
};

enum {
	RESULT_ACTIVATED,

	STARTED,
	FINISHED,
	CLEARED,

	N_SIGNALS
};

typedef struct
{
	PpsDocumentModel *model;
	PpsJobFind *job;
	PpsFindOptions options;
	PpsFindOptions supported_options;

	guint pages_searched;

	gchar *search_term;

	gint first_match_page;

	GtkSortListModel *sorted_model;
	GListStore *result_model;
} PpsSearchContextPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsSearchContext, pps_search_context, G_TYPE_OBJECT)

#define GET_PRIVATE(o) pps_search_context_get_instance_private (o)

static guint signals[N_SIGNALS];

static GParamSpec *props[NUM_PROPERTIES] = {
	NULL,
};

static void
pps_search_context_clear_job (PpsSearchContext *context)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	if (!priv->job)
		return;

	if (!pps_job_is_finished (PPS_JOB (priv->job)))
		pps_job_cancel (PPS_JOB (priv->job));

	g_signal_handlers_disconnect_matched (priv->job, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, context);
	g_clear_object (&priv->job);
}

static void
pps_search_context_set_supported_options (PpsSearchContext *context,
                                          PpsFindOptions options)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	priv->supported_options = options;
}

static void
pps_search_context_setup_document (PpsSearchContext *context,
                                   PpsDocument *document)
{
	if (!document || !PPS_IS_DOCUMENT_FIND (document)) {
		pps_search_context_set_supported_options (context, PPS_FIND_DEFAULT);
		return;
	}

	pps_search_context_set_supported_options (context, pps_document_find_get_supported_options (PPS_DOCUMENT_FIND (document)));
}

static void
document_changed_cb (PpsDocumentModel *model,
                     GParamSpec *pspec,
                     PpsSearchContext *context)
{
	pps_search_context_setup_document (context, pps_document_model_get_document (model));
}

static gchar *
sanitized_substring (const gchar *text,
                     gint start,
                     gint end)
{
	const gchar *p;
	const gchar *start_ptr;
	const gchar *end_ptr;
	guint len = 0;
	gchar *retval;

	if (end - start <= 0)
		return NULL;

	start_ptr = g_utf8_offset_to_pointer (text, start);
	end_ptr = g_utf8_offset_to_pointer (start_ptr, end - start);

	retval = g_malloc (end_ptr - start_ptr + 1);
	p = start_ptr;

	while (p != end_ptr) {
		const gchar *next;

		next = g_utf8_next_char (p);

		if (next != end_ptr) {
			GUnicodeBreakType break_type;

			break_type = g_unichar_break_type (g_utf8_get_char (p));
			if (break_type == G_UNICODE_BREAK_HYPHEN && *next == '\n') {
				p = g_utf8_next_char (next);
				continue;
			}
		}

		if (*p != '\n') {
			strncpy (retval + len, p, next - p);
			len += next - p;
		} else {
			*(retval + len) = ' ';
			len++;
		}

		p = next;
	}

	if (len == 0) {
		g_free (retval);

		return NULL;
	}

	retval[len] = 0;

	return retval;
}

static gchar *
get_surrounding_text_markup (const gchar *text,
                             const gchar *find_text,
                             gboolean case_sensitive,
                             PangoLogAttr *log_attrs,
                             gint log_attrs_length,
                             gint offset,
                             gboolean has_nextline,
                             gboolean hyphen_was_ignored)
{
	gint iter;
	gchar *prec = NULL;
	gchar *succ = NULL;
	gchar *match = NULL;
	gchar *markup;
	gint max_chars;

	iter = MAX (0, offset - 1);
	while (!log_attrs[iter].is_word_start && iter > 0)
		iter--;

	prec = sanitized_substring (text, iter, offset);

	iter = offset;
	offset += g_utf8_strlen (find_text, -1);

	if (has_nextline || g_utf8_offset_to_pointer (text, offset - 1)[0] == '\n') {
		if (has_nextline) {
			offset += 1; /* for newline */
			if (hyphen_was_ignored)
				offset += 1; /* for hyphen */
		}
		match = sanitized_substring (text, iter, offset);
	} else if (!case_sensitive)
		match = g_utf8_substring (text, iter, offset);

	iter = MIN (log_attrs_length, offset + 1);
	max_chars = MIN (log_attrs_length - 1, iter + 100);
	while (TRUE) {
		gint word = iter;

		while (!log_attrs[word].is_word_end && word < max_chars)
			word++;

		if (word > max_chars)
			break;

		iter = word + 1;
	}

	succ = sanitized_substring (text, offset, iter);

	markup = g_markup_printf_escaped ("%s<span weight=\"bold\">%s</span>%s",
	                                  prec ? prec : "", match ? match : find_text, succ ? succ : "");
	g_free (prec);
	g_free (succ);
	g_free (match);

	return markup;
}

static gchar *
get_page_text (PpsDocument *document,
               PpsPage *page,
               PpsRectangle **areas,
               guint *n_areas)
{
	gchar *text;
	gboolean success;

	pps_document_doc_mutex_lock (document);
	text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), page);
	success = pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document), page, areas, n_areas);
	pps_document_doc_mutex_unlock (document);

	if (!success) {
		g_free (text);
		return NULL;
	}

	return text;
}

static gint
get_match_offset (PpsRectangle *areas,
                  guint n_areas,
                  PpsFindRectangle *match,
                  gint offset)
{
	gdouble x, y;
	gint i;

	x = match->x1;
	y = (match->y1 + match->y2) / 2;

	i = offset;

	do {
		PpsRectangle *area = areas + i;
		gdouble area_y = (area->y1 + area->y2) / 2;
		gdouble area_x = (area->x1 + area->x2) / 2;

		if (x >= area->x1 && x < area->x2 &&
		    y >= area->y1 && y <= area->y2 &&
		    area_x >= match->x1 && area_x <= match->x2 &&
		    area_y >= match->y1 && area_y <= match->y2) {
			return i;
		}

		i = (i + 1) % n_areas;
	} while (i != offset);

	return -1;
}

static void
find_job_updated_cb (PpsJobFind *job,
                     gint current_page,
                     PpsSearchContext *context)
{
	GList *matches, *l;
	PpsPage *page;
	guint index;
	gchar *page_label;
	gchar *page_text;
	PpsRectangle *areas = NULL;
	guint n_areas;
	PangoLogAttr *text_log_attrs;
	gulong text_log_attrs_length;
	gint offset;
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);
	PpsDocument *document;

	g_autoptr (GPtrArray) results_array = g_ptr_array_new ();
	PpsSearchResult **results;
	gsize n_results;

	g_return_if_fail (PPS_IS_JOB (priv->job));

	document = pps_job_get_document (PPS_JOB (priv->job));

	matches = priv->job->pages[current_page];
	if (!matches)
		return;

	page = pps_document_get_page (document, current_page);
	page_label = pps_document_get_page_label (document, current_page);
	page_text = get_page_text (document, page, &areas, &n_areas);
	g_object_unref (page);
	if (!page_text)
		return;

	text_log_attrs_length = g_utf8_strlen (page_text, -1);
	text_log_attrs = g_new0 (PangoLogAttr, text_log_attrs_length + 1);
	pango_get_log_attrs (page_text, -1, -1, NULL, text_log_attrs, text_log_attrs_length + 1);

	if (priv->first_match_page == -1 && current_page >= priv->job->start_page)
		priv->first_match_page = current_page;

	offset = 0;

	for (l = matches, index = 0; l; l = g_list_next (l), index++) {
		PpsFindRectangle *match = (PpsFindRectangle *) l->data;
		PpsSearchResult *result;
		gchar *markup;
		gint new_offset;

		if (l->prev && ((PpsFindRectangle *) l->prev->data)->next_line)
			continue; /* Skip as this is second part of a multi-line match */

		new_offset = get_match_offset (areas, n_areas, match, offset);
		if (new_offset == -1) {
			/* It may happen that a text match has no corresponding text area available,
			 * (due to limitations/bugs of Poppler's TextPage->getSelectionWords() used by
			 * poppler-glib poppler_page_get_text_layout_for_area() function) so in that
			 * case we just show matched text because we cannot retrieve surrounding text.
			 * Issue #1943 and related #1545 */
			markup = g_strdup_printf ("<b>%s</b>", priv->job->text);
		} else {
			offset = new_offset;
			markup = get_surrounding_text_markup (page_text,
			                                      priv->job->text,
			                                      priv->job->options & PPS_FIND_CASE_SENSITIVE,
			                                      text_log_attrs,
			                                      text_log_attrs_length,
			                                      offset,
			                                      match->next_line,
			                                      match->after_hyphen);
		}

		result = pps_search_result_new (g_strdup (markup),
		                                g_strdup (page_label),
		                                current_page,
		                                index);

		g_ptr_array_add (results_array, result);

		g_free (markup);
	}

	g_free (page_label);
	g_free (page_text);
	g_free (text_log_attrs);
	g_free (areas);

	results = (PpsSearchResult **) g_ptr_array_steal (results_array, &n_results);
	if (n_results > 0)
		g_list_store_splice (priv->result_model, 0, 0, (gpointer *) results, (guint) n_results);

}

static void
find_job_finished_cb (PpsJobFind *job,
                      PpsSearchContext *context)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	g_signal_emit (context, signals[FINISHED], 0, priv->job, priv->first_match_page);

	pps_search_context_clear_job (context);
}

static void
search_changed_cb (PpsSearchContext *context)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	pps_search_context_clear_job (context);
	priv->pages_searched = 0;
	if (priv->result_model != NULL)
		g_list_store_remove_all (priv->result_model);

	if (priv->search_term && priv->search_term[0]) {
		PpsDocument *doc = pps_document_model_get_document (priv->model);

		priv->job = PPS_JOB_FIND (pps_job_find_new (doc,
		                                            pps_document_model_get_page (priv->model),
		                                            pps_document_get_n_pages (doc),
		                                            priv->search_term,
		                                            priv->options));
		priv->first_match_page = -1;
		g_signal_connect (priv->job, "updated",
		                  G_CALLBACK (find_job_updated_cb),
		                  context);
		g_signal_connect (priv->job, "finished",
		                  G_CALLBACK (find_job_finished_cb),
		                  context);

		g_signal_emit (context, signals[STARTED], 0, priv->job);
		pps_job_scheduler_push_job (PPS_JOB (priv->job), PPS_JOB_PRIORITY_NONE);
	} else {
		g_signal_emit (context, signals[CLEARED], 0);
	}
}

static void
pps_search_context_dispose (GObject *object)
{
	PpsSearchContext *context = PPS_SEARCH_CONTEXT (object);

	pps_search_context_clear_job (context);

	G_OBJECT_CLASS (pps_search_context_parent_class)->dispose (object);
}

static void
pps_search_context_init (PpsSearchContext *context)
{
}

static void
pps_search_context_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
	PpsSearchContext *context = PPS_SEARCH_CONTEXT (object);
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	switch (prop_id) {
	case PROP_DOCUMENT_MODEL:
		priv->model = g_value_get_object (value);
		break;
	case PROP_SEARCH_TERM:
		pps_search_context_set_search_term (context, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_search_context_get_property (GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
	PpsSearchContext *context = PPS_SEARCH_CONTEXT (object);
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	switch (prop_id) {
	case PROP_SEARCH_TERM:
		g_value_set_string (value, priv->search_term);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_search_context_constructed (GObject *object)
{
	PpsSearchContext *context = PPS_SEARCH_CONTEXT (object);
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	G_OBJECT_CLASS (pps_search_context_parent_class)->constructed (object);

	g_object_add_weak_pointer (G_OBJECT (priv->model),
	                           (gpointer) &priv->model);

	pps_search_context_setup_document (context, pps_document_model_get_document (priv->model));
	g_signal_connect_object (priv->model, "notify::document",
	                         G_CALLBACK (document_changed_cb),
	                         context, G_CONNECT_DEFAULT);
}

static void
pps_search_context_class_init (PpsSearchContextClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = pps_search_context_set_property;
	gobject_class->get_property = pps_search_context_get_property;
	gobject_class->dispose = pps_search_context_dispose;
	gobject_class->constructed = pps_search_context_constructed;

	props[PROP_DOCUMENT_MODEL] =
	    g_param_spec_object ("document-model",
	                         "DocumentModel",
	                         "The document model",
	                         PPS_TYPE_DOCUMENT_MODEL,
	                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	props[PROP_SEARCH_TERM] =
	    g_param_spec_string ("search-term",
	                         NULL,
	                         "search term for the current context",
	                         "",
	                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	g_object_class_install_properties (gobject_class, NUM_PROPERTIES, props);

	/* Signals */
	signals[RESULT_ACTIVATED] =
	    g_signal_new ("result-activated",
	                  G_TYPE_FROM_CLASS (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_generic,
	                  G_TYPE_NONE, 2,
	                  G_TYPE_INT,
	                  G_TYPE_INT);
	signals[STARTED] =
	    g_signal_new ("started",
	                  G_OBJECT_CLASS_TYPE (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_VOID__OBJECT,
	                  G_TYPE_NONE, 1,
	                  PPS_TYPE_JOB_FIND);
	signals[FINISHED] =
	    g_signal_new ("finished",
	                  G_OBJECT_CLASS_TYPE (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_generic,
	                  G_TYPE_NONE, 2,
	                  PPS_TYPE_JOB_FIND,
	                  G_TYPE_INT);
	signals[CLEARED] =
	    g_signal_new ("cleared",
	                  G_OBJECT_CLASS_TYPE (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, 0);
}

PpsSearchContext *
pps_search_context_new (PpsDocumentModel *model)
{
	return PPS_SEARCH_CONTEXT (g_object_new (PPS_TYPE_SEARCH_CONTEXT,
	                                         "search-term", "",
	                                         "document-model", model,
	                                         NULL));
}

void
pps_search_context_set_search_term (PpsSearchContext *context,
                                    const gchar *search_term)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	if (g_strcmp0 (search_term, priv->search_term) == 0)
		return;

	priv->search_term = g_strdup (search_term);
	g_object_notify_by_pspec (G_OBJECT (context), props[PROP_SEARCH_TERM]);
	search_changed_cb (context);
}

const gchar *
pps_search_context_get_search_term (PpsSearchContext *context)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	return g_strdup (priv->search_term);
}

void
pps_search_context_set_options (PpsSearchContext *context,
                                PpsFindOptions options)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	if (priv->options == options)
		return;

	priv->options = options;
	search_changed_cb (context);
}

PpsFindOptions
pps_search_context_get_options (PpsSearchContext *context)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	return priv->options;
}

static gint
sort_func (gconstpointer a,
           gconstpointer b,
           gpointer      user_data)
{
	PpsSearchResult *result_a = (gpointer) a;
	PpsSearchResult *result_b = (gpointer) b;
	guint page_a = pps_search_result_get_page (result_a);
	guint page_b = pps_search_result_get_page (result_b);

	if (page_a == page_b)
		return pps_search_result_get_index (result_a) - pps_search_result_get_index (result_b);
	else
		return page_a - page_b;
}

/**
 * pps_search_context_get_result_model:
 *
 * Returns: (transfer none): the returned #GListModel
 */
GListModel *
pps_search_context_get_result_model (PpsSearchContext *context)
{
	g_return_val_if_fail (PPS_IS_SEARCH_CONTEXT (context), 0);

	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	if (priv->result_model == NULL) {
		GtkCustomSorter *custom_sorter = gtk_custom_sorter_new (sort_func, NULL, NULL);
		priv->result_model = g_list_store_new (PPS_TYPE_SEARCH_RESULT);
		priv->sorted_model = gtk_sort_list_model_new (G_LIST_MODEL (priv->result_model),
							      GTK_SORTER (custom_sorter));
	}

	return G_LIST_MODEL (priv->sorted_model);
}

void
pps_search_context_restart (PpsSearchContext *context)
{
	search_changed_cb (context);
}

void
pps_search_context_select_result (PpsSearchContext *context,
                                  PpsSearchResult *result)
{
	g_signal_emit (context, signals[RESULT_ACTIVATED], 0,
	               pps_search_result_get_page (result),
	               pps_search_result_get_index (result));
}
