// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 *  Copyright (C) 2005 Red Hat, Inc
 */

#pragma once

#include <libdocument/pps-macros.h>
#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <gio/gio.h>

#include <papers-document.h>

#define PPS_GET_TYPE_NAME(instance) g_type_name_from_instance ((gpointer) instance)

G_BEGIN_DECLS

#define PPS_TYPE_JOB (pps_job_get_type ())

PPS_PUBLIC
G_DECLARE_DERIVABLE_TYPE (PpsJob, pps_job, PPS, JOB, GObject)

struct _PpsJobClass {
	GObjectClass parent_class;

	void (*run) (PpsJob *job);

	/* Signals */
	void (*cancelled) (PpsJob *job);
	void (*finished) (PpsJob *job);
};

PPS_PUBLIC
void pps_job_run (PpsJob *job);
PPS_PUBLIC
void pps_job_cancel (PpsJob *job);
void pps_job_failed (PpsJob *job,
                     GQuark domain,
                     gint code,
                     const gchar *format,
                     ...) G_GNUC_PRINTF (4, 5);
void pps_job_failed_from_error (PpsJob *job,
                                GError *error);
void pps_job_succeeded (PpsJob *job);
PPS_PUBLIC
gboolean pps_job_is_finished (PpsJob *job);
PPS_PUBLIC
gboolean pps_job_is_succeeded (PpsJob *job,
                               GError **error);
PPS_PUBLIC
PpsDocument *pps_job_get_document (PpsJob *job);
GCancellable *pps_job_get_cancellable (PpsJob *job);

PPS_PUBLIC
void pps_job_reset (PpsJob *job);

G_END_DECLS
