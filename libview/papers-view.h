// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2009 Christian Persch
 */

#pragma once

#define __PPS_PAPERS_VIEW_H_INSIDE__

#ifndef I_KNOW_THE_PAPERS_LIBS_ARE_UNSTABLE_AND_HAVE_TALKED_WITH_THE_AUTHORS
#error You have to define I_KNOW_THE_PAPERS_LIBS_ARE_UNSTABLE_AND_HAVE_TALKED_WITH_THE_AUTHORS. And please! Contact the authors
#endif

#include <libview/context/pps-annotations-context.h>
#include <libview/context/pps-attachment-context.h>
#include <libview/context/pps-document-model.h>
#include <libview/context/pps-history.h>
#include <libview/context/pps-metadata.h>
#include <libview/context/pps-search-context.h>
#include <libview/context/pps-search-result.h>

#include <libview/pps-job-scheduler.h>
#include <libview/pps-job.h>
#include <libview/pps-jobs.h>
#include <libview/pps-print-operation.h>
#include <libview/pps-view-type-builtins.h>
#include <libview/pps-view.h>

#undef __PPS_PAPERS_VIEW_H_INSIDE__
