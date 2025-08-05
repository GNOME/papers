/* pps-attachment-context.c
 *  this file is part of papers, a gnome document viewer
 *
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

#include <glib/gi18n.h>

#include "pps-attachment-context.h"
#include "pps-jobs.h"
#include <papers-view.h>

enum {
	PROP_0,
	PROP_DOCUMENT_MODEL,
	NUM_PROPERTIES
};

typedef struct
{
	PpsDocumentModel *document_model;
	PpsJobAttachments *job;
	GListStore *attachment_model;
} PpsAttachmentContextPrivate;

typedef struct
{
	PpsAttachmentContext *context;
	GListModel *attachments;
} PpsAttachmentContextSaveData;

G_DEFINE_TYPE_WITH_PRIVATE (PpsAttachmentContext, pps_attachment_context, G_TYPE_OBJECT)

G_DEFINE_QUARK (pps - attachment - context - error - quark, pps_attachment_context_error)

#define GET_PRIVATE(o) pps_attachment_context_get_instance_private (o)

static GParamSpec *props[NUM_PROPERTIES] = {
	NULL,
};

static void
pps_attachment_context_clear_job (PpsAttachmentContext *context)
{
	PpsAttachmentContextPrivate *priv = GET_PRIVATE (context);

	if (!priv->job)
		return;

	if (!pps_job_is_finished (PPS_JOB (priv->job)))
		pps_job_cancel (PPS_JOB (priv->job));

	g_signal_handlers_disconnect_matched (priv->job, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, context);
	g_clear_object (&priv->job);
}

static void
attachments_job_finished_cb (PpsJobAttachments *job,
                             PpsAttachmentContext *context)
{
	PpsAttachmentContextPrivate *priv = GET_PRIVATE (context);
	g_autoptr (GPtrArray) attachments_array = g_ptr_array_new ();
	g_autofree PpsAttachment **attachments;
	GList *attachments_list = pps_job_attachments_get_attachments (job);
	gsize n_attachments;

	for (GList *l = attachments_list; l && l->data; l = g_list_next (l)) {
		g_ptr_array_add (attachments_array, l->data);
	}

	attachments = (PpsAttachment **) g_ptr_array_steal (attachments_array, &n_attachments);
	if (n_attachments > 0)
		g_list_store_splice (priv->attachment_model, 0, 0, (gpointer *) attachments, (guint) n_attachments);

	pps_attachment_context_clear_job (context);
}

static void
pps_attachment_context_setup_document (PpsAttachmentContext *context,
                                       PpsDocument *document)
{
	PpsAttachmentContextPrivate *priv = GET_PRIVATE (context);

	g_list_store_remove_all (priv->attachment_model);

	if (!PPS_IS_DOCUMENT_ATTACHMENTS (document))
		return;

	if (!pps_document_attachments_has_attachments (PPS_DOCUMENT_ATTACHMENTS (document)))
		return;

	pps_attachment_context_clear_job (context);

	priv->job = PPS_JOB_ATTACHMENTS (pps_job_attachments_new (document));
	g_signal_connect (priv->job, "finished",
	                  G_CALLBACK (attachments_job_finished_cb),
	                  context);
	g_signal_connect_swapped (priv->job, "cancelled",
	                          G_CALLBACK (pps_attachment_context_clear_job),
	                          context);

	pps_job_scheduler_push_job (PPS_JOB (priv->job), PPS_JOB_PRIORITY_NONE);
}

static void
document_changed_cb (PpsDocumentModel *model,
                     GParamSpec *pspec,
                     PpsAttachmentContext *context)
{
	pps_attachment_context_setup_document (context, pps_document_model_get_document (model));
}

static void
pps_attachment_context_dispose (GObject *object)
{
	PpsAttachmentContext *context = PPS_ATTACHMENT_CONTEXT (object);
	PpsAttachmentContextPrivate *priv = GET_PRIVATE (context);

	pps_attachment_context_clear_job (context);
	g_clear_object (&priv->attachment_model);

	G_OBJECT_CLASS (pps_attachment_context_parent_class)->dispose (object);
}

static void
pps_attachment_context_init (PpsAttachmentContext *context)
{
	PpsAttachmentContextPrivate *priv = GET_PRIVATE (context);

	priv->attachment_model = g_list_store_new (PPS_TYPE_ATTACHMENT);
}

static void
pps_attachment_context_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
	PpsAttachmentContext *context = PPS_ATTACHMENT_CONTEXT (object);
	PpsAttachmentContextPrivate *priv = GET_PRIVATE (context);

	switch (prop_id) {
	case PROP_DOCUMENT_MODEL:
		priv->document_model = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_attachment_context_constructed (GObject *object)
{
	PpsAttachmentContext *context = PPS_ATTACHMENT_CONTEXT (object);
	PpsAttachmentContextPrivate *priv = GET_PRIVATE (context);

	G_OBJECT_CLASS (pps_attachment_context_parent_class)->constructed (object);

	g_object_add_weak_pointer (G_OBJECT (priv->document_model),
	                           (gpointer) &priv->document_model);

	pps_attachment_context_setup_document (context, pps_document_model_get_document (priv->document_model));
	g_signal_connect_object (priv->document_model, "notify::document",
	                         G_CALLBACK (document_changed_cb),
	                         context, G_CONNECT_DEFAULT);
}

static void
pps_attachment_context_class_init (PpsAttachmentContextClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = pps_attachment_context_set_property;
	gobject_class->dispose = pps_attachment_context_dispose;
	gobject_class->constructed = pps_attachment_context_constructed;

	props[PROP_DOCUMENT_MODEL] =
	    g_param_spec_object ("document-model",
	                         "DocumentModel",
	                         "The document model",
	                         PPS_TYPE_DOCUMENT_MODEL,
	                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (gobject_class, NUM_PROPERTIES, props);
}

PpsAttachmentContext *
pps_attachment_context_new (PpsDocumentModel *model)
{
	return PPS_ATTACHMENT_CONTEXT (g_object_new (PPS_TYPE_ATTACHMENT_CONTEXT,
	                                             "document-model", model,
	                                             NULL));
}

/**
 * pps_attachment_context_get_model:
 *
 * Returns: (transfer none) (not nullable): the returned #GListModel of #PpsAttachment s
 */
GListModel *
pps_attachment_context_get_model (PpsAttachmentContext *context)
{
	PpsAttachmentContextPrivate *priv = GET_PRIVATE (context);

	return G_LIST_MODEL (priv->attachment_model);
}

static void
free_save_data (PpsAttachmentContextSaveData *save_data)
{
	g_clear_object (&save_data->context);
	g_clear_object (&save_data->attachments);
	g_clear_pointer (&save_data, g_free);
}

static void
save_attachment_to_target_file (PpsAttachment *attachment,
                                GFile *target_file,
                                gboolean is_dir,
                                GError **error)
{
	g_autoptr (GFile) save_to = NULL;
	gboolean is_native = g_file_is_native (target_file);

	if (!is_native) {
		/*
		 * This feature was dropped as being unnecessary, and only
		 * adding to the complexity of this.
		 *
		 * Saving to a remote non-native location would involve saving
		 * it to a temp file, and copying that over to the non-native
		 * location.
		 */
		*error = g_error_new (PPS_ATTACHMENT_CONTEXT_ERROR,
		                      PPS_ATTACHMENT_CONTEXT_ERROR_NOT_IMPLEMENTED,
		                      "Saving to remote locations is not implemented");
		return;
	}

	if (is_dir)
		save_to = g_file_get_child (target_file,
		                            /* FIXME chpe: file name encoding! */
		                            pps_attachment_get_name (attachment));
	else
		save_to = g_object_ref (target_file);

	if (save_to)
		pps_attachment_save (attachment, save_to, error);
}

static void
attachments_save_dialog_response_cb (GtkFileDialog *dialog,
                                     GAsyncResult *result,
                                     GTask *task)
{
	PpsAttachmentContextSaveData *save_data = g_task_get_task_data (task);
	gboolean is_dir = g_list_model_get_n_items (save_data->attachments) != 1;
	g_autoptr (GFile) target_file;
	GError *error = NULL;
	guint i = 0;

	if (is_dir)
		target_file = gtk_file_dialog_select_folder_finish (dialog, result, &error);
	else
		target_file = gtk_file_dialog_save_finish (dialog, result, &error);

	for (i = 0; !error && i < g_list_model_get_n_items (save_data->attachments); i++) {
		g_autoptr (PpsAttachment) attachment =
		    g_list_model_get_item (save_data->attachments, i);
		save_attachment_to_target_file (attachment, target_file, is_dir, &error);
	}

	if (error)
		g_task_return_error (task, error);
	else
		g_task_return_boolean (task, TRUE);

	g_object_unref (task);
}

/**
 * pps_attachment_context_save_attachments_async:
 * @context: a #PpsAttachmentContext
 * @parent: (nullable): the parent `GtkWindow`
 * @attachments: (transfer full): The attachments to save
 * @cancellable: (nullable): a `GCancellable` to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * This function initiates a file save operation.
 *
 * The @callback will be called when the dialog is dismissed.
 */
void
pps_attachment_context_save_attachments_async (PpsAttachmentContext *context,
                                               GListModel *attachments,
                                               GtkWindow *parent,
                                               GCancellable *cancellable,
                                               GAsyncReadyCallback callback,
                                               gpointer user_data)
{
	g_autoptr (GtkFileDialog) dialog = NULL;
	g_autoptr (GTask) task = NULL;
	PpsAttachmentContextSaveData *save_data;
	g_autoptr (PpsAttachment) first_attachment = NULL;

	g_assert (g_type_is_a (g_list_model_get_item_type (attachments), PPS_TYPE_ATTACHMENT));

	g_return_if_fail (PPS_IS_ATTACHMENT_CONTEXT (context));

	save_data = g_new (PpsAttachmentContextSaveData, 1);
	task = g_task_new (context, cancellable, callback, user_data);

	save_data->context = g_object_ref (context);
	save_data->attachments = attachments;

	g_task_set_task_data (task, save_data, (GDestroyNotify) free_save_data);

	if (g_list_model_get_n_items (attachments) == 0) {
		g_task_return_error (task,
		                     g_error_new (PPS_ATTACHMENT_CONTEXT_ERROR,
		                                  PPS_ATTACHMENT_CONTEXT_ERROR_EMPTY_INPUT,
		                                  "No attachment was selected"));
		return;
	}

	dialog = gtk_file_dialog_new ();

	gtk_file_dialog_set_title (dialog, ngettext ("Save Attachment", "Save Attachments", g_list_model_get_n_items (attachments)));
	gtk_file_dialog_set_modal (dialog, TRUE);

	if (g_list_model_get_n_items (attachments) == 1) {
		first_attachment = g_list_model_get_item (attachments, 0);
		gtk_file_dialog_set_initial_name (dialog, pps_attachment_get_name (first_attachment));

		gtk_file_dialog_save (dialog, parent, cancellable,
		                      (GAsyncReadyCallback) attachments_save_dialog_response_cb,
		                      g_steal_pointer (&task));
	} else {
		gtk_file_dialog_select_folder (dialog, parent, cancellable,
		                               (GAsyncReadyCallback) attachments_save_dialog_response_cb,
		                               g_steal_pointer (&task));
	}
}

/**
 * pps_attachment_context_save_attachments_finish:
 * @context: a #PpsAttachmentContext
 * @result: a `GAsyncResult`
 * @error: return location for an error
 *
 * Finishes the [method@Pps.AttachmentContext.save_attachments_async] call
 *
 * Returns: whether a files were stored
 */
gboolean
pps_attachment_context_save_attachments_finish (PpsAttachmentContext *context,
                                                GAsyncResult *result,
                                                GError **error)
{
	g_return_val_if_fail (g_task_is_valid (result, context), FALSE);

	return g_task_propagate_boolean (G_TASK (result), error);
}
