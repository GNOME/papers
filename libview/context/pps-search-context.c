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
	PROP_ACTIVE,

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

	gchar *search_term;
	guint active_use_count;

	gboolean autoselecting;

	GHashTable *per_page_store;
	GListStore *result_model;
	GtkSingleSelection *selection_model;
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

typedef struct {
	PpsSearchContext *context;
	gint page;
} ContextPage;

static void
process_updates (ContextPage *data)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (data->context);
	g_autoptr (GPtrArray) results_array = g_ptr_array_new ();
	PpsSearchResult **results;
	GList *results_list = pps_job_find_get_results (priv->job)[data->page];
	GPtrArray *per_page_results_array = g_ptr_array_new_null_terminated (0, g_object_unref, TRUE);
	gsize n_results;

	g_return_if_fail (PPS_IS_JOB (priv->job));

	for (GList *l = results_list; l && l->data; l = g_list_next (l)) {
		PpsSearchResult *result = l->data;
		g_ptr_array_add (per_page_results_array, g_object_ref (result));
		g_ptr_array_add (results_array, result);
	}

	g_hash_table_insert (priv->per_page_store, GINT_TO_POINTER (data->page), per_page_results_array);

	results = (PpsSearchResult **) g_ptr_array_steal (results_array, &n_results);
	if (n_results > 0)
		// The first 0 is position, we probably need to change it
		g_list_store_splice (priv->result_model, 0, 0, (gpointer *) results, (guint) n_results);

	g_free (data);
}

static void
find_job_updated_cb (PpsJobFind *job,
                     gint page,
                     PpsSearchContext *context)
{
	ContextPage *data = g_new (ContextPage, 1);
	data->context = context;
	data->page = page;
	g_idle_add_once ((GSourceOnceFunc) process_updates, data);
}

static void
find_job_finished_cb (PpsJobFind *job,
                      PpsSearchContext *context)
{
	// TODO: Fix 34
	g_signal_emit (context, signals[FINISHED], 0, 34);

	pps_search_context_clear_job (context);
}

static void
selection_changed_cb (PpsSearchContext *context)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);
	PpsSearchResult *result = PPS_SEARCH_RESULT (gtk_single_selection_get_selected_item (priv->selection_model));

	if (result != NULL && !priv->autoselecting)
		g_signal_emit (context, signals[RESULT_ACTIVATED], 0, result);
}

static void
pps_search_context_dispose (GObject *object)
{
	PpsSearchContext *context = PPS_SEARCH_CONTEXT (object);
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	pps_search_context_clear_job (context);
	g_clear_object (&priv->per_page_store);
	g_clear_object (&priv->result_model);
	g_clear_object (&priv->selection_model);

	G_OBJECT_CLASS (pps_search_context_parent_class)->dispose (object);
}

static void
pps_search_context_init (PpsSearchContext *context)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	priv->per_page_store = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) g_ptr_array_unref);
	priv->result_model = g_list_store_new (PPS_TYPE_SEARCH_RESULT);
	priv->selection_model = gtk_single_selection_new (G_LIST_MODEL (priv->result_model));
	gtk_single_selection_set_autoselect (priv->selection_model, FALSE);
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
	case PROP_ACTIVE:
		g_value_set_boolean (value, pps_search_context_get_active (context));
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

	g_signal_connect_object (priv->selection_model, "selection-changed",
	                         G_CALLBACK (selection_changed_cb), context,
	                         G_CONNECT_SWAPPED);
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

	props[PROP_ACTIVE] =
	    g_param_spec_boolean ("active",
	                          NULL,
	                          "search is actively ongoing",
	                          FALSE,
	                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	g_object_class_install_properties (gobject_class, NUM_PROPERTIES, props);

	/* Signals */
	signals[RESULT_ACTIVATED] =
	    g_signal_new ("result-activated",
	                  G_TYPE_FROM_CLASS (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_generic,
	                  G_TYPE_NONE, 1,
	                  PPS_TYPE_SEARCH_RESULT);
	signals[STARTED] =
	    g_signal_new ("started",
	                  G_OBJECT_CLASS_TYPE (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, 0);
	signals[FINISHED] =
	    g_signal_new ("finished",
	                  G_OBJECT_CLASS_TYPE (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_VOID__INT,
	                  G_TYPE_NONE, 1,
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
	pps_search_context_restart (context);
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
	pps_search_context_restart (context);
}

PpsFindOptions
pps_search_context_get_options (PpsSearchContext *context)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	return priv->options;
}

/**
 * pps_search_context_get_result_model:
 *
 * Returns: (not nullable) (transfer none): the returned #GtkSingleSelection
 */
GtkSingleSelection *
pps_search_context_get_result_model (PpsSearchContext *context)
{
	g_return_val_if_fail (PPS_IS_SEARCH_CONTEXT (context), NULL);

	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	return priv->selection_model;
}

/**
 * pps_search_context_get_results_on_page:
 *
 * Returns: (not nullable) (transfer full) (element-type PpsSearchResult): #GPtrArray of #PpsSearchResult on the current page
 *
 * Since: 48.0
 */
GPtrArray *
pps_search_context_get_results_on_page (PpsSearchContext *context,
                                        guint page)
{
	g_return_val_if_fail (PPS_IS_SEARCH_CONTEXT (context), NULL);

	PpsSearchContextPrivate *priv = GET_PRIVATE (context);
	GPtrArray *result_array = (GPtrArray *) g_hash_table_lookup (priv->per_page_store, GINT_TO_POINTER (page));

	if (result_array == NULL)
		return g_ptr_array_new_null_terminated (0, g_object_unref, TRUE);

	return g_ptr_array_copy (result_array, (GCopyFunc) g_object_ref, NULL);
}

gboolean
pps_search_context_has_results_on_page (PpsSearchContext *context,
                                        guint page)
{
	g_return_val_if_fail (PPS_IS_SEARCH_CONTEXT (context), false);

	PpsSearchContextPrivate *priv = GET_PRIVATE (context);
	GPtrArray *result_array = (GPtrArray *) g_hash_table_lookup (priv->per_page_store, GINT_TO_POINTER (page));

	return result_array != NULL && result_array->len > 0;
}

/**
 * pps_search_context_activate:
 *
 * Since: 48.0
 */
void
pps_search_context_activate (PpsSearchContext *context)
{
	g_return_if_fail (PPS_IS_SEARCH_CONTEXT (context));

	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	priv->active_use_count++;

	if (priv->active_use_count == 1) {
		g_object_notify_by_pspec (G_OBJECT (context), props[PROP_ACTIVE]);

		/*
		 * If the list model was not filled for a search term that was
		 * set before search was activated, it means we have cancelled
		 * the search job and we need to restart it.
		 *
		 * Otherwise, we can pretend to run a search job, mocking it
		 * with emitting the same signals, because the model is still
		 * up-to-date.
		 */
		if (priv->search_term && priv->search_term[0]) {
			if (g_list_model_get_n_items (G_LIST_MODEL (priv->result_model)) == 0) {
				pps_search_context_restart (context);
			} else {
				g_signal_emit (context, signals[STARTED], 0);
				g_signal_emit (context, signals[FINISHED], 0, -1);
			}
		}
	}
}

/**
 * pps_search_context_release:
 *
 * Since: 48.0
 */
void
pps_search_context_release (PpsSearchContext *context)
{
	g_return_if_fail (PPS_IS_SEARCH_CONTEXT (context));

	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	g_return_if_fail (priv->active_use_count > 0);

	priv->active_use_count--;

	if (priv->active_use_count == 0) {
		g_object_notify_by_pspec (G_OBJECT (context), props[PROP_ACTIVE]);

		pps_search_context_clear_job (context);
	}
}

/**
 * pps_search_context_get_active:
 *
 * Returns: (not nullable): whether search is actively ongoing
 *
 * Since: 48.0
 */
gboolean
pps_search_context_get_active (PpsSearchContext *context)
{
	g_return_val_if_fail (PPS_IS_SEARCH_CONTEXT (context), FALSE);

	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	return priv->active_use_count > 0;
}

void
pps_search_context_restart (PpsSearchContext *context)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	pps_search_context_clear_job (context);
	g_hash_table_remove_all (priv->per_page_store);
	g_list_store_remove_all (priv->result_model);

	if (priv->search_term && priv->search_term[0]) {
		PpsDocument *doc = pps_document_model_get_document (priv->model);

		priv->job = PPS_JOB_FIND (pps_job_find_new (doc,
		                                            pps_document_model_get_page (priv->model),
		                                            pps_document_get_n_pages (doc),
		                                            priv->search_term,
		                                            priv->options));
		g_signal_connect (priv->job, "finished",
		                  G_CALLBACK (find_job_finished_cb),
		                  context);
		g_signal_connect (priv->job, "updated",
		                  G_CALLBACK (find_job_updated_cb),
		                  context);

		g_signal_emit (context, signals[STARTED], 0);
		pps_job_scheduler_push_job (PPS_JOB (priv->job), PPS_JOB_PRIORITY_NONE);
	} else {
		g_signal_emit (context, signals[CLEARED], 0);
	}
}

/**
 * pps_search_context_autoselect_result:
 * @result: a #PpsSearchResult to be the auto-selected one
 *
 * Since: 48.0
 */
void
pps_search_context_autoselect_result (PpsSearchContext *context,
                                      PpsSearchResult *result)
{
	PpsSearchContextPrivate *priv = GET_PRIVATE (context);

	priv->autoselecting = TRUE;
	gtk_single_selection_set_selected (priv->selection_model,
	                                   pps_search_result_get_global_index (result));
	priv->autoselecting = FALSE;
}
