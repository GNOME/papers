// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2005 Marco Pesenti Gritti
 *  Copyright (C) 2018 Germán Poo-Caamaño <gpoo@gnome.org>
 */

#include "config.h"

#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#include "pps-history.h"

enum {
	CHANGED,
	ACTIVATE_LINK,

	N_SIGNALS
};

enum {
	PROP_0,
	PROP_DOCUMENT_MODEL,
	NUM_PROPERTIES
};

#define PPS_HISTORY_MAX_LENGTH (32)

static guint signals[N_SIGNALS] = {
	0,
};
static GParamSpec *props[NUM_PROPERTIES] = {
	NULL,
};

typedef struct
{
	GList *list;
	GList *current;

	PpsDocumentModel *model;

	guint frozen;
} PpsHistoryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsHistory, pps_history, G_TYPE_OBJECT)

#define GET_PRIVATE(o) pps_history_get_instance_private (o);

static void pps_history_set_model (PpsHistory *history,
                                   PpsDocumentModel *model);
static gint pps_history_get_current_page (PpsHistory *history);

static void
pps_history_clear (PpsHistory *history)
{
	PpsHistoryPrivate *priv = GET_PRIVATE (history);

	g_clear_list (&priv->list, g_object_unref);

	priv->current = NULL;
}

static void
pps_history_prune (PpsHistory *history)
{
	PpsHistoryPrivate *priv = GET_PRIVATE (history);
	GList *l;
	guint i;

	g_assert (priv->current->next == NULL);

	for (i = 0, l = priv->current; i < PPS_HISTORY_MAX_LENGTH && l != NULL; i++, l = l->prev)
		/* empty */;

	if (l == NULL)
		return;

	/* Throw away all history up to @l */
	l = l->next;
	l->prev->next = NULL;
	l->prev = NULL;

	g_clear_list (&priv->list, g_object_unref);

	priv->list = l;

	g_assert (g_list_length (priv->list) == PPS_HISTORY_MAX_LENGTH);
}

static void
pps_history_finalize (GObject *object)
{
	PpsHistory *history = PPS_HISTORY (object);

	pps_history_clear (history);
	pps_history_set_model (history, NULL);

	G_OBJECT_CLASS (pps_history_parent_class)->finalize (object);
}

static void
pps_history_set_property (GObject *object,
                          guint prop_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
	PpsHistory *history = PPS_HISTORY (object);

	switch (prop_id) {
	case PROP_DOCUMENT_MODEL:
		pps_history_set_model (history, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_history_class_init (PpsHistoryClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = pps_history_finalize;
	object_class->set_property = pps_history_set_property;

	signals[CHANGED] =
	    g_signal_new ("changed",
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	                  G_STRUCT_OFFSET (PpsHistoryClass, changed),
	                  NULL, NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, 0);

	signals[ACTIVATE_LINK] =
	    g_signal_new ("activate-link",
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	                  G_STRUCT_OFFSET (PpsHistoryClass, activate_link),
	                  NULL, NULL,
	                  g_cclosure_marshal_VOID__OBJECT,
	                  G_TYPE_NONE, 1,
	                  G_TYPE_OBJECT);

	props[PROP_DOCUMENT_MODEL] =
	    g_param_spec_object ("document-model",
	                         "DocumentModel",
	                         "The document model",
	                         PPS_TYPE_DOCUMENT_MODEL,
	                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, NUM_PROPERTIES, props);
}

static void
pps_history_init (PpsHistory *history)
{
}

gboolean
pps_history_is_frozen (PpsHistory *history)
{
	PpsHistoryPrivate *priv = GET_PRIVATE (history);

	return priv->frozen > 0;
}

void
pps_history_add_link (PpsHistory *history,
                      PpsLink *link)
{
	PpsHistoryPrivate *priv;

	g_return_if_fail (PPS_IS_HISTORY (history));
	g_return_if_fail (PPS_IS_LINK (link));

	if (pps_history_is_frozen (history))
		return;

	priv = GET_PRIVATE (history);

	if (priv->current) {
		/* Truncate forward history at @current */
		g_clear_list (&priv->current->next, g_object_unref);
	}

	/* Push @link to the list */
	priv->current = g_list_append (NULL, g_object_ref (link));
	priv->list = g_list_concat (priv->list, priv->current);

	pps_history_prune (history);

	g_signal_emit (history, signals[CHANGED], 0);
}

static void
pps_history_activate_current_link (PpsHistory *history)
{
	PpsHistoryPrivate *priv = GET_PRIVATE (history);

	g_assert (priv->current);

	pps_history_freeze (history);
	g_signal_emit (history, signals[ACTIVATE_LINK], 0, priv->current->data);
	pps_history_thaw (history);

	g_signal_emit (history, signals[CHANGED], 0);
}

gboolean
pps_history_can_go_back (PpsHistory *history)
{
	PpsHistoryPrivate *priv;

	g_return_val_if_fail (PPS_IS_HISTORY (history), FALSE);

	priv = GET_PRIVATE (history);

	if (pps_history_is_frozen (history))
		return FALSE;

	if (abs (pps_document_model_get_page (priv->model) - pps_history_get_current_page (history)) > 1)
		return TRUE;

	return priv->current && priv->current->prev;
}

void
pps_history_go_back (PpsHistory *history)
{
	PpsHistoryPrivate *priv;

	g_return_if_fail (PPS_IS_HISTORY (history));

	if (!pps_history_can_go_back (history))
		return;

	priv = GET_PRIVATE (history);

	/* Move current back one step */
	priv->current = priv->current->prev;

	pps_history_activate_current_link (history);
}

gboolean
pps_history_can_go_forward (PpsHistory *history)
{
	PpsHistoryPrivate *priv;

	g_return_val_if_fail (PPS_IS_HISTORY (history), FALSE);

	if (pps_history_is_frozen (history))
		return FALSE;

	priv = GET_PRIVATE (history);
	return priv->current && priv->current->next;
}

void
pps_history_go_forward (PpsHistory *history)
{
	PpsHistoryPrivate *priv;

	g_return_if_fail (PPS_IS_HISTORY (history));

	if (!pps_history_can_go_forward (history))
		return;

	priv = GET_PRIVATE (history);

	/* Move current forward one step */
	priv->current = priv->current->next;

	pps_history_activate_current_link (history);
}

static gint
compare_link (PpsLink *a,
              PpsLink *b)
{
	PpsLinkAction *aa, *bb;

	if (a == b)
		return 0;

	aa = pps_link_get_action (a);
	bb = pps_link_get_action (b);

	return (aa && bb && pps_link_action_equal (aa, bb)) ? 0 : 1;
}

/*
 * pps_history_go_to_link:
 * @history: a #PpsHistory
 * @link: a #PpsLink
 *
 * Goes to the link, if it is in the history.
 *
 * Returns: %TRUE if the link was in the history and history isn't frozen; %FALSE otherwise
 */
gboolean
pps_history_go_to_link (PpsHistory *history,
                        PpsLink *link)
{
	PpsHistoryPrivate *priv;
	GList *l;

	g_return_val_if_fail (PPS_IS_HISTORY (history), FALSE);
	g_return_val_if_fail (PPS_IS_LINK (link), FALSE);

	if (pps_history_is_frozen (history))
		return FALSE;

	priv = GET_PRIVATE (history);

	l = g_list_find_custom (priv->list, link, (GCompareFunc) compare_link);
	if (l == NULL)
		return FALSE;

	/* Set the link as current */
	priv->current = l;

	pps_history_activate_current_link (history);

	return TRUE;
}

/**
 * pps_history_get_back_list:
 * @history: a #PpsHistory
 *
 * Returns: (transfer container) (element-type PpsLink): the back history
 */
GList *
pps_history_get_back_list (PpsHistory *history)
{
	PpsHistoryPrivate *priv;
	GList *list, *l;

	g_return_val_if_fail (PPS_IS_HISTORY (history), NULL);

	priv = GET_PRIVATE (history);

	if (priv->current == NULL)
		return NULL;

	list = NULL;
	for (l = priv->current->prev; l != NULL; l = l->prev)
		list = g_list_prepend (list, l->data);

	return g_list_reverse (list);
}

/**
 * pps_history_get_forward_list:
 * @history: a #PpsHistory
 *
 * Returns: (transfer container) (element-type PpsLink): the forward history
 */
GList *
pps_history_get_forward_list (PpsHistory *history)
{
	PpsHistoryPrivate *priv;

	g_return_val_if_fail (PPS_IS_HISTORY (history), NULL);

	priv = GET_PRIVATE (history);

	return g_list_copy (priv->current->next);
}

void
pps_history_freeze (PpsHistory *history)
{
	PpsHistoryPrivate *priv;

	g_return_if_fail (PPS_IS_HISTORY (history));

	priv = GET_PRIVATE (history);

	priv->frozen++;
}

void
pps_history_thaw (PpsHistory *history)
{
	PpsHistoryPrivate *priv;

	g_return_if_fail (PPS_IS_HISTORY (history));

	priv = GET_PRIVATE (history);

	g_return_if_fail (priv->frozen > 0);

	priv->frozen--;
}

static gint
pps_history_get_current_page (PpsHistory *history)
{
	PpsHistoryPrivate *priv = GET_PRIVATE (history);
	PpsLink *link;
	PpsDocument *document;
	PpsLinkDest *dest;
	PpsLinkAction *action;

	if (!priv->current)
		return -1;

	link = priv->current->data;
	action = pps_link_get_action (link);
	if (!action)
		return -1;

	dest = pps_link_action_get_dest (action);
	if (!dest)
		return -1;

	switch (pps_link_dest_get_dest_type (dest)) {
	case PPS_LINK_DEST_TYPE_NAMED:
		document = pps_document_model_get_document (priv->model);
		if (!PPS_IS_DOCUMENT_LINKS (document))
			return -1;

		return pps_document_links_find_link_page (PPS_DOCUMENT_LINKS (document),
		                                          pps_link_dest_get_named_dest (dest));
	case PPS_LINK_DEST_TYPE_PAGE_LABEL: {
		gint page = -1;

		document = pps_document_model_get_document (priv->model);
		pps_document_find_page_by_label (document,
		                                 pps_link_dest_get_page_label (dest),
		                                 &page);

		return page;
	}
	default:
		return pps_link_dest_get_page (dest);
	}

	return -1;
}

void
pps_history_add_page (PpsHistory *history,
                      gint page)
{
	PpsHistoryPrivate *priv = GET_PRIVATE (history);
	PpsDocument *document;
	PpsLinkDest *dest;
	PpsLinkAction *action;
	PpsLink *link;
	gchar *page_label;
	gchar *title;

	if (pps_history_is_frozen (history))
		return;

	if (pps_history_get_current_page (history) == page)
		return;

	document = pps_document_model_get_document (priv->model);
	if (!document)
		return;

	page_label = pps_document_get_page_label (document, page);
	if (!page_label)
		return;

	title = g_strdup_printf (_ ("Page %s"), page_label);
	g_free (page_label);

	dest = pps_link_dest_new_page (page);
	action = pps_link_action_new_dest (dest);
	g_object_unref (dest);

	link = pps_link_new (title, action);
	g_object_unref (action);
	g_free (title);

	pps_history_add_link (history, link);
	g_object_unref (link);
}

static void
document_changed_cb (PpsDocumentModel *model,
                     GParamSpec *pspec,
                     PpsHistory *history)
{
	pps_history_clear (history);
	pps_history_add_page (history, pps_document_model_get_page (model));
}

static void
pps_history_set_model (PpsHistory *history,
                       PpsDocumentModel *model)
{
	PpsHistoryPrivate *priv = GET_PRIVATE (history);

	if (priv->model == model)
		return;

	if (priv->model) {
		g_object_remove_weak_pointer (G_OBJECT (priv->model),
		                              (gpointer) &priv->model);
	}

	priv->model = model;
	if (!model)
		return;

	g_object_add_weak_pointer (G_OBJECT (model),
	                           (gpointer) &priv->model);

	g_signal_connect (priv->model, "notify::document",
	                  G_CALLBACK (document_changed_cb),
	                  history);
}

PpsHistory *
pps_history_new (PpsDocumentModel *model)
{
	PpsHistory *history;

	g_return_val_if_fail (PPS_IS_DOCUMENT_MODEL (model), NULL);

	history = PPS_HISTORY (g_object_new (PPS_TYPE_HISTORY, NULL));
	pps_history_set_model (history, model);

	return history;
}
