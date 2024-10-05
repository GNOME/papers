/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 *  Copyright (C) 2005 Red Hat, Inc
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

#pragma once

#include <libdocument/pps-macros.h>
#if !defined (__PPS_PAPERS_VIEW_H_INSIDE__) && !defined (PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <gio/gio.h>

#include <papers-document.h>

#define PPS_GET_TYPE_NAME(instance) g_type_name_from_instance ((gpointer)instance)

G_BEGIN_DECLS

#define PPS_TYPE_JOB            (pps_job_get_type())

PPS_PUBLIC
G_DECLARE_DERIVABLE_TYPE (PpsJob, pps_job, PPS, JOB, GObject)

struct _PpsJobClass
{
	GObjectClass parent_class;

	gboolean (*run)         (PpsJob *job);

	/* Signals */
	void     (* cancelled)  (PpsJob *job);
	void     (* finished)   (PpsJob *job);
};


PPS_PUBLIC
gboolean        pps_job_run                (PpsJob          *job);
PPS_PUBLIC
void            pps_job_cancel             (PpsJob          *job);
PPS_PUBLIC
void            pps_job_failed             (PpsJob          *job,
					   GQuark          domain,
					   gint            code,
					   const gchar    *format,
					   ...) G_GNUC_PRINTF (4, 5);
PPS_PUBLIC
void            pps_job_failed_from_error  (PpsJob          *job,
					   GError         *error);
PPS_PUBLIC
void            pps_job_succeeded          (PpsJob          *job);
PPS_PUBLIC
gboolean        pps_job_is_finished        (PpsJob          *job);
PPS_PUBLIC
gboolean        pps_job_is_succeeded       (PpsJob          *job,
					    GError         **error);
PPS_PUBLIC
PpsDocument     *pps_job_get_document	   (PpsJob	  *job);
PPS_PUBLIC
GCancellable    *pps_job_get_cancellable   (PpsJob	  *job);

PPS_PUBLIC
void		pps_job_reset		   (PpsJob *job);

G_END_DECLS
