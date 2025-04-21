// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-job-scheduler.h
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#pragma once

#if !defined(__PPS_PAPERS_VIEW_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-view.h> can be included directly."
#endif

#include <glib.h>

#include "pps-jobs.h"
#include <papers-document.h>

G_BEGIN_DECLS

typedef enum {
	PPS_JOB_PRIORITY_URGENT, /* Rendering current page range */
	PPS_JOB_PRIORITY_HIGH,   /* Rendering current thumbnail range */
	PPS_JOB_PRIORITY_LOW,    /* Rendering pages not in current range */
	PPS_JOB_PRIORITY_NONE,   /* Any other job: load, save, print, ... */
	PPS_JOB_N_PRIORITIES
} PpsJobPriority;

PPS_PUBLIC
void pps_job_scheduler_push_job (PpsJob *job,
                                 PpsJobPriority priority);
PPS_PUBLIC
void pps_job_scheduler_update_job (PpsJob *job,
                                   PpsJobPriority priority);
PPS_PUBLIC
void pps_job_scheduler_wait (void);

G_END_DECLS
