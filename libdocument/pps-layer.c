// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#include <config.h>

#include "pps-layer.h"
#include <gio/gio.h>

enum {
	PROP_0,
	PROP_CHILDREN,
	PROP_ENABLED,
	PROP_TITLE_ONLY,
	PROP_TITLE,
};

struct _PpsLayerPrivate {
	gint rb_group;
	GListModel *children;
	gboolean enabled;
	gboolean title_only;
	gchar *title;
};

typedef struct _PpsLayerPrivate PpsLayerPrivate;

#define GET_PRIVATE(o) pps_layer_get_instance_private (o);

G_DEFINE_TYPE_WITH_PRIVATE (PpsLayer, pps_layer, G_TYPE_OBJECT)

gint
pps_layer_get_rb_group (PpsLayer *layer)
{
	g_return_val_if_fail (PPS_IS_LAYER (layer), 0);
	PpsLayerPrivate *priv = GET_PRIVATE (layer);

	return priv->rb_group;
}

/**
 * pps_layer_set_children:
 * @pps_layer: A #PpsLayer
 * @children: (transfer full): The children of the layer
 *
 * Sets the 'children' property of the layer.
 */
void
pps_layer_set_children (PpsLayer *pps_layer, GListModel *children)
{
	PpsLayerPrivate *priv = GET_PRIVATE (pps_layer);
	g_return_if_fail (PPS_IS_LAYER (pps_layer));

	if (g_set_object (&priv->children, children))
		g_object_notify (G_OBJECT (pps_layer), "children");
}

/**
 * pps_layer_get_children:
 * @pps_layer: A #PpsLayer
 *
 * Returns: (nullable) (transfer none): The children of the layer
 */
GListModel *
pps_layer_get_children (PpsLayer *pps_layer)
{
	PpsLayerPrivate *priv = GET_PRIVATE (pps_layer);
	g_return_val_if_fail (PPS_IS_LAYER (pps_layer), NULL);

	return priv->children;
}

static void
pps_layer_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
	PpsLayer *layer = PPS_LAYER (object);
	PpsLayerPrivate *priv = GET_PRIVATE (layer);

	switch (prop_id) {
	case PROP_CHILDREN:
		g_value_set_object (value, priv->children);
		break;
	case PROP_ENABLED:
		g_value_set_boolean (value, priv->enabled);
		break;
	case PROP_TITLE_ONLY:
		g_value_set_boolean (value, priv->title_only);
		break;
	case PROP_TITLE:
		g_value_set_string (value, priv->title);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
pps_layer_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
	PpsLayer *layer = PPS_LAYER (object);
	PpsLayerPrivate *priv = GET_PRIVATE (layer);

	switch (prop_id) {
	case PROP_CHILDREN:
		priv->children = g_value_get_object (value);
		break;
	case PROP_ENABLED:
		priv->enabled = g_value_get_boolean (value);
		break;
	case PROP_TITLE_ONLY:
		priv->title_only = g_value_get_boolean (value);
		break;
	case PROP_TITLE:
		priv->title = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
pps_layer_dispose (GObject *object)
{
	PpsLayer *layer = PPS_LAYER (object);
	PpsLayerPrivate *priv = GET_PRIVATE (layer);

	g_clear_object (&priv->children);

	G_OBJECT_CLASS (pps_layer_parent_class)->dispose (object);
}

static void
pps_layer_class_init (PpsLayerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = pps_layer_set_property;
	object_class->get_property = pps_layer_get_property;
	object_class->dispose = pps_layer_dispose;

	g_object_class_install_property (object_class,
	                                 PROP_CHILDREN,
	                                 g_param_spec_object ("children",
	                                                      "Children",
	                                                      "The children of the layer",
	                                                      G_TYPE_LIST_MODEL,
	                                                      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
	                                 PROP_ENABLED,
	                                 g_param_spec_boolean ("enabled",
	                                                       "Enabled",
	                                                       "Whether the layer is enabled",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE |
	                                                           G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_TITLE_ONLY,
	                                 g_param_spec_boolean ("title-only",
	                                                       "Title Only",
	                                                       "Whether the layer is title only",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE |
	                                                           G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_TITLE,
	                                 g_param_spec_string ("title",
	                                                      "Title",
	                                                      "Name of the layer",
	                                                      FALSE,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_STATIC_STRINGS));
}

static void
pps_layer_init (PpsLayer *layer)
{
}

PpsLayer *
pps_layer_new (gint rb_group)
{
	PpsLayer *layer;
	PpsLayerPrivate *priv;

	layer = PPS_LAYER (g_object_new (PPS_TYPE_LAYER, NULL));
	priv = GET_PRIVATE (layer);
	priv->rb_group = rb_group;

	return layer;
}
