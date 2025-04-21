// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 *  Copyright (C) 2005 Red Hat, Inc
 */

#include <config.h>

#include "pps-debug.h"
#include "pps-job.h"

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "PpsJobs"

enum {
	PROP_0,
	PROP_DOCUMENT,
};

enum {
	CANCELLED,
	FINISHED,
	LAST_SIGNAL
};

static guint job_signals[LAST_SIGNAL] = { 0 };

typedef struct _PpsJobPrivate {
	PpsDocument *document;

	guint finished : 1;
	guint failed : 1;

	GError *error;
	GCancellable *cancellable;

	guint idle_finished_id;
	guint idle_cancelled_id;
} PpsJobPrivate;

#define GET_PRIVATE(o) pps_job_get_instance_private (o)

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (PpsJob, pps_job, G_TYPE_OBJECT)

static void
pps_job_init (PpsJob *job)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);
	priv->cancellable = g_cancellable_new ();
}

static void
pps_job_dispose (GObject *object)
{
	PpsJob *job = PPS_JOB (object);
	PpsJobPrivate *priv = GET_PRIVATE (job);

	g_debug ("disposing %s (%p)", PPS_GET_TYPE_NAME (job), job);

	g_clear_object (&priv->document);
	g_clear_object (&priv->cancellable);
	g_clear_error (&priv->error);

	(*G_OBJECT_CLASS (pps_job_parent_class)->dispose) (object);
}

static void
pps_job_set_property (GObject *object,
                      guint prop_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
	PpsJob *job = PPS_JOB (object);
	PpsJobPrivate *priv = GET_PRIVATE (job);

	switch (prop_id) {
	case PROP_DOCUMENT:
		priv->document = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
pps_job_class_init (PpsJobClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->dispose = pps_job_dispose;
	g_object_class->set_property = pps_job_set_property;

	job_signals[CANCELLED] =
	    g_signal_new ("cancelled",
	                  PPS_TYPE_JOB,
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (PpsJobClass, cancelled),
	                  NULL, NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, 0);
	job_signals[FINISHED] =
	    g_signal_new ("finished",
	                  PPS_TYPE_JOB,
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (PpsJobClass, finished),
	                  NULL, NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, 0);

	g_object_class_install_property (g_object_class,
	                                 PROP_DOCUMENT,
	                                 g_param_spec_object ("document",
	                                                      "Document",
	                                                      "The document",
	                                                      PPS_TYPE_DOCUMENT,
	                                                      G_PARAM_WRITABLE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
}

static void
emit_finished (PpsJob *job)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	g_debug ("emit finished for %s (%p)", PPS_GET_TYPE_NAME (job), job);

	priv->idle_finished_id = 0;

	if (g_cancellable_is_cancelled (priv->cancellable))
		g_debug ("%s (%p) job was cancelled, do not emit finished", PPS_GET_TYPE_NAME (job), job);
	else
		g_signal_emit (job, job_signals[FINISHED], 0);

	g_object_unref (job);
}

static void
pps_job_emit_finished (PpsJob *job)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	if (g_cancellable_is_cancelled (priv->cancellable)) {
		g_debug ("%s (%p) job was cancelled, returning", PPS_GET_TYPE_NAME (job), job);
		return;
	}

	priv->finished = TRUE;

	priv->idle_finished_id =
	    g_idle_add_once ((GSourceOnceFunc) emit_finished,
	                     g_object_ref (job));
}

/**
 * pps_job_run:
 * @job: A #PpsJob object to run.
 *
 * Starts or executes the specified job.
 */
void
pps_job_run (PpsJob *job)
{
	PpsJobClass *class = PPS_JOB_GET_CLASS (job);

	PPS_PROFILER_START (PPS_GET_TYPE_NAME (job), g_strdup ("running"));
	class->run (job);
	PPS_PROFILER_STOP ();
}

/**
 * pps_job_cancel:
 * @job: A #PpsJob object to cancel.
 *
 * Cancels the specified job if it has not already been cancelled.
 *
 * This function updates the job's state to indicate it has been cancelled,
 * cancels any associated cancellable object, and emits a CANCELLED signal.
 * It does nothing if the job is already cancelled or finished.
 *
 * This function should not be called from a thread.
 */
void
pps_job_cancel (PpsJob *job)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	if (g_cancellable_is_cancelled (priv->cancellable))
		return;

	g_debug ("job %s (%p) cancelled", PPS_GET_TYPE_NAME (job), job);

	/* This should never be called from a thread */
	g_cancellable_cancel (priv->cancellable);

	if (priv->finished && priv->idle_finished_id == 0)
		return;

	g_signal_emit (job, job_signals[CANCELLED], 0);
}

/**
 * pps_job_failed:
 * @job: A #PpsJob object.
 * @domain: The error domain (a #GQuark).
 * @code: The error code (an integer).
 * @format: A printf-style format string.
 *
 * Marks the specified job as failed and sets an error with the provided details.
 *
 * This function creates a new #GError using the provided domain, code, and message,
 * and updates the job's state to indicate failure. It emits a finished signal when done.
 * It does nothing if the job is already marked as failed or finished.
 */
void
pps_job_failed (PpsJob *job,
                GQuark domain,
                gint code,
                const gchar *format,
                ...)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);
	va_list args;
	gchar *message;

	if (priv->failed || priv->finished)
		return;

	g_debug ("job %s (%p) failed", PPS_GET_TYPE_NAME (job), job);

	priv->failed = TRUE;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	priv->error = g_error_new_literal (domain, code, message);
	g_free (message);

	pps_job_emit_finished (job);
}

/**
 * pps_job_failed_from_error: (rename-to pps_job_failed)
 * @job: A #PpsJob object.
 * @error: A #GError containing details about the failure.
 *
 * Marks the specified job as failed and sets the error associated with the failure.
 *
 * This function updates the job's state to indicate it has failed, stores the error,
 * and emits a finished signal. It does nothing if the job is already finished or marked as failed.
 */
void
pps_job_failed_from_error (PpsJob *job,
                           GError *error)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	if (priv->failed || priv->finished)
		return;

	g_debug ("job %s (%p) failed", PPS_GET_TYPE_NAME (job), job);

	priv->failed = TRUE;
	priv->error = g_error_copy (error);

	pps_job_emit_finished (job);
}

/**
 * pps_job_succeeded:
 * @job: A #PpsJob object that has completed.
 *
 * Marks the specified job as succeeded.
 *
 * This function should be called when the job finishes successfully.
 */
void
pps_job_succeeded (PpsJob *job)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	if (priv->finished)
		return;

	g_debug ("job %s (%p) succeeded", PPS_GET_TYPE_NAME (job), job);

	priv->failed = FALSE;
	pps_job_emit_finished (job);
}

/**
 * pps_job_is_finished:
 * @job: A #PpsJob object.
 *
 * Checks if the specified job has finished execution.
 *
 * Returns: TRUE if the job is finished, FALSE otherwise.
 */
gboolean
pps_job_is_finished (PpsJob *job)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	return priv->finished;
}

/**
 * pps_job_is_succeeded:
 * @job: A #PpsJob object.
 * @error: (nullable) (transfer full): the error to set if the job fails.
 *
 * Determines the finish status of the specified job.
 *
 * Returns: TRUE if the job succeeded, FALSE otherwise.
 */
gboolean
pps_job_is_succeeded (PpsJob *job, GError **error)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	g_return_val_if_fail (job != NULL, FALSE);

	if (priv->failed && error)
		*error = g_error_copy (priv->error);

	return !priv->failed;
}

/**
 * pps_job_get_document:
 * @job: A #PpsJob object.
 *
 * Retrieves the #PpsDocument associated with the specified job.
 *
 * Returns: (transfer none): The #PpsDocument object associated with the job,
 * or NULL if the job is invalid or does not have an associated document.
 */
PpsDocument *
pps_job_get_document (PpsJob *job)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	g_return_val_if_fail (PPS_IS_JOB (job), NULL);

	return priv->document;
}

/**
 * pps_job_get_cancellable:
 * @job: A #PpsJob object.
 *
 * Retrieves the #GCancellable object associated with the specified job.
 *
 * Returns: (transfer none): The #GCancellable object of the job,
 * or NULL if the job is invalid or does not have an associated cancellable.
 */
GCancellable *
pps_job_get_cancellable (PpsJob *job)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	g_return_val_if_fail (PPS_IS_JOB (job), NULL);

	return priv->cancellable;
}

/**
 * pps_job_reset:
 * @job: A #PpsJob object to reset.
 *
 * Resets the state of the specified job to its initial state.
 *
 * This function clears any error, marks the job as not finished, and sets
 * the job's failure status to FALSE, effectively resetting its progress.
 */
void
pps_job_reset (PpsJob *job)
{
	PpsJobPrivate *priv = GET_PRIVATE (job);

	priv->failed = FALSE;
	priv->finished = FALSE;
	g_clear_error (&priv->error);
}
