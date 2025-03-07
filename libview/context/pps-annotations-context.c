/* pps-annotations-context.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Pablo Correa Gomez <ablocorrea@hotmail.com>
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

#include <papers-view.h>

#define ANNOT_POPUP_WINDOW_DEFAULT_WIDTH 200
#define ANNOT_POPUP_WINDOW_DEFAULT_HEIGHT 150
#define ANNOTATION_ICON_SIZE 24

enum {
	SIGNAL_ANNOT_ADDED,
	SIGNAL_ANNOT_REMOVED,
	SIGNAL_ANNOTS_LOADED,
	N_SIGNALS
};

static guint signals[N_SIGNALS];

enum {
	PROP_0,
	PROP_DOCUMENT_MODEL,
	NUM_PROPERTIES
};

typedef struct
{
	PpsDocumentModel *model;
	PpsJobAnnots *job;

	GListStore *annots_model;
} PpsAnnotationsContextPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsAnnotationsContext, pps_annotations_context, G_TYPE_OBJECT)

#define GET_PRIVATE(o) pps_annotations_context_get_instance_private (o)

static GParamSpec *props[NUM_PROPERTIES] = {
	NULL,
};

static void
pps_annotations_context_clear_job (PpsAnnotationsContext *self)
{
	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);

	if (!priv->job)
		return;

	/* this function may be a callback of the cancelled signal of priv->job,
	so we have to make sure it is disconnected before continuing before continuing */
	g_signal_handlers_disconnect_matched (priv->job, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, self);

	if (!pps_job_is_finished (PPS_JOB (priv->job)))
		pps_job_cancel (PPS_JOB (priv->job));

	g_clear_object (&priv->job);
}

static void
annotations_job_finished_cb (PpsJobAnnots *job,
                             PpsAnnotationsContext *self)
{
	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);
	g_autoptr (GPtrArray) annotations_array = g_ptr_array_new ();
	gpointer *annotations;
	GList *annotations_list = pps_job_annots_get_annots (job);
	gsize n_annotations;

	for (GList *l = annotations_list; l && l->data; l = g_list_next (l)) {
		g_ptr_array_add (annotations_array, l->data);
	}

	annotations = g_ptr_array_steal (annotations_array, &n_annotations);
	if (n_annotations > 0)
		g_list_store_splice (priv->annots_model, 0, 0, annotations, (guint) n_annotations);

	pps_annotations_context_clear_job (self);

	g_signal_emit (self, signals[SIGNAL_ANNOTS_LOADED], 0);
}

static void
pps_annotations_context_setup_document (PpsAnnotationsContext *self,
                                        PpsDocument *document)
{
	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);

	g_list_store_remove_all (priv->annots_model);

	if (!PPS_IS_DOCUMENT_ANNOTATIONS (document))
		return;

	pps_annotations_context_clear_job (self);

	priv->job = PPS_JOB_ANNOTS (pps_job_annots_new (document));
	g_signal_connect (priv->job, "finished",
	                  G_CALLBACK (annotations_job_finished_cb),
	                  self);
	g_signal_connect_swapped (priv->job, "cancelled",
	                          G_CALLBACK (pps_annotations_context_clear_job),
	                          self);

	pps_job_scheduler_push_job (PPS_JOB (priv->job), PPS_JOB_PRIORITY_NONE);
}

static void
document_changed_cb (PpsDocumentModel *model,
                     GParamSpec *pspec,
                     PpsAnnotationsContext *self)
{
	pps_annotations_context_setup_document (self, pps_document_model_get_document (model));
}

static void
pps_annotations_context_init (PpsAnnotationsContext *self)
{
	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);

	priv->annots_model = g_list_store_new (PPS_TYPE_ANNOTATION);
}

static void
pps_annotations_context_dispose (GObject *object)
{
	PpsAnnotationsContext *self = PPS_ANNOTATIONS_CONTEXT (object);
	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);

	pps_annotations_context_clear_job (self);
	g_clear_object (&priv->annots_model);

	G_OBJECT_CLASS (pps_annotations_context_parent_class)->dispose (object);
}

static void
pps_annotations_context_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
	PpsAnnotationsContext *self = PPS_ANNOTATIONS_CONTEXT (object);
	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_DOCUMENT_MODEL:
		priv->model = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_annotations_context_constructed (GObject *object)
{
	PpsAnnotationsContext *self = PPS_ANNOTATIONS_CONTEXT (object);
	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);

	G_OBJECT_CLASS (pps_annotations_context_parent_class)->constructed (object);

	g_object_add_weak_pointer (G_OBJECT (priv->model),
	                           (gpointer) &priv->model);

	pps_annotations_context_setup_document (self, pps_document_model_get_document (priv->model));
	g_signal_connect_object (priv->model, "notify::document",
	                         G_CALLBACK (document_changed_cb),
	                         self, G_CONNECT_DEFAULT);
}

static void
pps_annotations_context_class_init (PpsAnnotationsContextClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = pps_annotations_context_set_property;
	gobject_class->dispose = pps_annotations_context_dispose;
	gobject_class->constructed = pps_annotations_context_constructed;

	props[PROP_DOCUMENT_MODEL] =
	    g_param_spec_object ("document-model",
	                         "DocumentModel",
	                         "The document model",
	                         PPS_TYPE_DOCUMENT_MODEL,
	                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (gobject_class, NUM_PROPERTIES, props);

	signals[SIGNAL_ANNOT_ADDED] =
	    g_signal_new ("annot-added",
	                  G_TYPE_FROM_CLASS (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_generic,
	                  G_TYPE_NONE, 1,
	                  PPS_TYPE_ANNOTATION);

	signals[SIGNAL_ANNOT_REMOVED] =
	    g_signal_new ("annot-removed",
	                  G_TYPE_FROM_CLASS (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_generic,
	                  G_TYPE_NONE, 1,
	                  PPS_TYPE_ANNOTATION);

	signals[SIGNAL_ANNOTS_LOADED] =
	    g_signal_new ("annots-loaded",
	                  G_TYPE_FROM_CLASS (gobject_class),
	                  G_SIGNAL_RUN_LAST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_generic,
	                  G_TYPE_NONE, 0);
}

PpsAnnotationsContext *
pps_annotations_context_new (PpsDocumentModel *model)
{
	return PPS_ANNOTATIONS_CONTEXT (g_object_new (PPS_TYPE_ANNOTATIONS_CONTEXT,
	                                              "document-model", model,
	                                              NULL));
}

/**
 * pps_annotations_context_get_annots_model:
 * @self: a #PpsAnnotationsContext
 *
 * Returns: (not nullable) (transfer none): the returned #GListModel. The model
 * is owned but the `PpsAnnotationsContext` and shall not be modified outside
 * of it.
 */
GListModel *
pps_annotations_context_get_annots_model (PpsAnnotationsContext *self)
{
	g_return_val_if_fail (PPS_IS_ANNOTATIONS_CONTEXT (self), NULL);

	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);

	return G_LIST_MODEL (priv->annots_model);
}

static int
compare_annot (const PpsAnnotation *a,
               const PpsAnnotation *b,
               gpointer user_data)
{
	guint index_a = pps_annotation_get_page_index ((PpsAnnotation *) a);
	guint index_b = pps_annotation_get_page_index ((PpsAnnotation *) b);

	if (index_a == index_b)
		return 0;

	return index_a > index_b ? 1 : -1;
}

/**
 * pps_annotations_context_add_annotation_sync:
 * @self: a #PpsAnnotationsContext
 * @page_index: the index of the page where the annotation will be added
 * @type: the type of annotation to add
 * @start: point where to start creating an annotation
 * @end: point where to end creating the annotation. It is ignored for TEXT
 * annotations
 * @color: the color to give to the annotation
 * @user_data: a pointer with auxiliary data that is annotation-dependent
 *
 * Add an annotation based on the provided information.
 *
 * Returns: (transfer none): the newly created annotation
 *
 * Since: 48.0
 *
 */
PpsAnnotation *
pps_annotations_context_add_annotation_sync (PpsAnnotationsContext *self,
                                             gint page_index,
                                             PpsAnnotationType type,
                                             const PpsPoint *start,
                                             const PpsPoint *end,
                                             const GdkRGBA *color,
                                             const gpointer user_data)
{
	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);
	PpsDocument *document = pps_document_model_get_document (priv->model);
	PpsAnnotation *annot;
	PpsRectangle doc_rect;
	PpsPage *page;

	g_return_val_if_fail (PPS_IS_ANNOTATIONS_CONTEXT (self), NULL);

	pps_document_doc_mutex_lock (document);
	page = pps_document_get_page (document, page_index);
	pps_document_doc_mutex_unlock (document);
	switch (type) {
	case PPS_ANNOTATION_TYPE_TEXT:
		doc_rect.x1 = start->x - ANNOTATION_ICON_SIZE / 2;
		doc_rect.y1 = start->y - ANNOTATION_ICON_SIZE / 2;
		doc_rect.x2 = start->x + ANNOTATION_ICON_SIZE / 2;
		doc_rect.y2 = start->y + ANNOTATION_ICON_SIZE / 2;
		annot = pps_annotation_text_new (page);
		break;
	case PPS_ANNOTATION_TYPE_TEXT_MARKUP:
		doc_rect.x1 = start->x;
		doc_rect.y1 = start->y;
		doc_rect.x2 = end->x;
		doc_rect.y2 = end->y;
		annot = pps_annotation_text_markup_new (page, *(PpsAnnotationTextMarkupType *) user_data);
		break;
	default:
		g_assert_not_reached ();
		return NULL;
	}

	pps_annotation_set_area (annot, &doc_rect);
	pps_annotation_set_rgba (annot, color);

	g_object_set (annot,
	              "popup-is-open", FALSE,
	              "label", g_get_real_name (),
	              "opacity", 1.0,
	              NULL);

	pps_document_doc_mutex_lock (document);
	pps_document_annotations_add_annotation (PPS_DOCUMENT_ANNOTATIONS (document), annot);
	pps_document_doc_mutex_unlock (document);

	g_list_store_insert_sorted (priv->annots_model, annot,
	                            (GCompareDataFunc) compare_annot, NULL);

	g_signal_emit (self, signals[SIGNAL_ANNOT_ADDED], 0, annot);

	return annot;
}

void
pps_annotations_context_remove_annotation (PpsAnnotationsContext *self,
                                           PpsAnnotation *annot)
{
	PpsAnnotationsContextPrivate *priv = GET_PRIVATE (self);
	PpsDocument *document = pps_document_model_get_document (priv->model);
	guint position;

	g_return_if_fail (PPS_IS_ANNOTATIONS_CONTEXT (self));
	g_return_if_fail (PPS_IS_ANNOTATION (annot));

	pps_document_doc_mutex_lock (document);
	pps_document_annotations_remove_annotation (PPS_DOCUMENT_ANNOTATIONS (document),
	                                            annot);
	pps_document_doc_mutex_unlock (document);

	if (!g_list_store_find (priv->annots_model, annot, &position))
		g_assert_not_reached ();

	g_list_store_remove (priv->annots_model, position);
	g_signal_emit (self, signals[SIGNAL_ANNOT_REMOVED], 0, annot);
}
