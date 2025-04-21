// SPDX-License-Identifier: LGPL-2.1-or-later
/* this file is part of papers, a gnome document viewer
 *
 * Copyright © 2009 Christian Persch
 */

#pragma once

#if !defined(__PPS_PAPERS_DOCUMENT_H_INSIDE__) && !defined(PAPERS_COMPILATION)
#error "Only <papers-document.h> can be included directly."
#endif

#include <glib.h>

#include "pps-macros.h"

G_BEGIN_DECLS

PPS_PUBLIC
gboolean pps_init (void);

PPS_PUBLIC
void pps_shutdown (void);

gboolean _pps_is_initialized (void);

G_END_DECLS
