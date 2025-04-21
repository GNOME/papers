// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-document-layers.c
 *  this file is part of papers, a gnome document_links viewer
 *
 * Copyright (C) 2008 Carlos Garcia Campos  <carlosgc@gnome.org>
 */

#include "config.h"

#include "pps-document-layers.h"
#include "pps-document.h"

G_DEFINE_INTERFACE (PpsDocumentLayers, pps_document_layers, 0)

static void
pps_document_layers_default_init (PpsDocumentLayersInterface *klass)
{
}

gboolean
pps_document_layers_has_layers (PpsDocumentLayers *document_layers)
{
	PpsDocumentLayersInterface *iface = PPS_DOCUMENT_LAYERS_GET_IFACE (document_layers);

	return iface->has_layers (document_layers);
}

/**
 * pps_document_layers_get_layers:
 * @document_layers: an #PpsDocumentLayers
 *
 * Returns: (transfer full): a #GListModel
 */
GListModel *
pps_document_layers_get_layers (PpsDocumentLayers *document_layers)
{
	PpsDocumentLayersInterface *iface = PPS_DOCUMENT_LAYERS_GET_IFACE (document_layers);

	return iface->get_layers (document_layers);
}

void
pps_document_layers_show_layer (PpsDocumentLayers *document_layers,
                                PpsLayer *layer)
{
	PpsDocumentLayersInterface *iface = PPS_DOCUMENT_LAYERS_GET_IFACE (document_layers);

	iface->show_layer (document_layers, layer);
}

void
pps_document_layers_hide_layer (PpsDocumentLayers *document_layers,
                                PpsLayer *layer)
{
	PpsDocumentLayersInterface *iface = PPS_DOCUMENT_LAYERS_GET_IFACE (document_layers);

	iface->hide_layer (document_layers, layer);
}

gboolean
pps_document_layers_layer_is_visible (PpsDocumentLayers *document_layers,
                                      PpsLayer *layer)
{
	PpsDocumentLayersInterface *iface = PPS_DOCUMENT_LAYERS_GET_IFACE (document_layers);

	return iface->layer_is_visible (document_layers, layer);
}
