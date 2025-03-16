// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2005, Red Hat, Inc.
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <gtk/gtk.h>

#include "pps-document.h"
#include "pps-macros.h"

G_BEGIN_DECLS

gboolean _pps_document_factory_init (void);
void _pps_document_factory_shutdown (void);

PPS_PUBLIC
PpsDocument *pps_document_factory_get_document (const char *uri, GError **error);
PPS_PUBLIC
PpsDocument *pps_document_factory_get_document_for_fd (int fd,
                                                       const char *mime_type,
                                                       GError **error);

PPS_PUBLIC
void pps_document_factory_add_filters (GtkFileDialog *dialog, PpsDocument *document);

G_END_DECLS
