// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2006 Pauli Virtanen <pav@iki.fi>
 */

#pragma once

#include "djvu-document.h"
#include "pps-document-links.h"

#include <glib.h>

GListModel *djvu_links_get_links_model (PpsDocumentLinks *document_links);
PpsMappingList *djvu_links_get_links (PpsDocumentLinks *document_links,
                                      gint page,
                                      double scale_factor);
PpsLinkDest *djvu_links_find_link_dest (PpsDocumentLinks *document_links,
                                        const gchar *link_name);
gint djvu_links_find_link_page (PpsDocumentLinks *document_links,
                                const gchar *link_name);
gboolean djvu_links_has_document_links (PpsDocumentLinks *document_links);
