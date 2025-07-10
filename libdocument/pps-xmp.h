// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright © 2021 Christian Persch
 */

#pragma once

#if !defined(PAPERS_COMPILATION)
#error "This is a private header."
#endif

#include "pps-document-info.h"

G_BEGIN_DECLS

gboolean pps_xmp_parse (const char *metadata,
                        gsize size,
                        PpsDocumentInfo *info);

G_END_DECLS
