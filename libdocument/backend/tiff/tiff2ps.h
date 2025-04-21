// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005 rpath, Inc.
 */

#pragma once

#include "tiffio.h"
#include <glib.h>
#include <stdio.h>

typedef struct _TIFF2PSContext TIFF2PSContext;

TIFF2PSContext *tiff2ps_context_new (const gchar *filename);
void tiff2ps_process_page (TIFF2PSContext *ctx, TIFF *tif, double pagewidth, double pageheight, double leftmargin, double bottommargin, gboolean center);
void tiff2ps_context_finalize (TIFF2PSContext *ctx);
