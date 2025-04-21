// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2006 Michael Hofmann <mh21@piware.de>
 */

#pragma once

#include "pps-document.h"

#include <glib.h>
#include <libdjvu/miniexp.h>
#include <string.h>

typedef struct _DjvuTextPage DjvuTextPage;
typedef struct _DjvuTextLink DjvuTextLink;

struct _DjvuTextPage {
	char *text;
	GArray *links;
	GList *results;
	miniexp_t char_symbol;
	miniexp_t word_symbol;
	PpsRectangle *bounding_box;
	miniexp_t text_structure;
	miniexp_t start;
	miniexp_t end;
};

struct _DjvuTextLink {
	int position;
	miniexp_t pair;
};

typedef enum {
	DJVU_SELECTION_TEXT,
	DJVU_SELECTION_BOX,
} DjvuSelectionType;

GList *djvu_text_page_get_selection_region (DjvuTextPage *page,
                                            PpsRectangle *rectangle);
char *djvu_text_page_copy (DjvuTextPage *page,
                           PpsRectangle *rectangle);
void djvu_text_page_index_text (DjvuTextPage *page,
                                gboolean case_sensitive);
void djvu_text_page_search (DjvuTextPage *page,
                            const char *text);
DjvuTextPage *djvu_text_page_new (miniexp_t text);
void djvu_text_page_free (DjvuTextPage *page);
