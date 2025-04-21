// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2022 Qiu Wenbo
 */

#include "pps-outlines.h"

struct _PpsOutlinesPrivate {
	gchar *markup;
	gchar *label;
	gboolean expand;
	PpsLink *link;
	GListModel *children;
};

typedef struct _PpsOutlinesPrivate PpsOutlinesPrivate;
#define GET_PRIVATE(o) pps_outlines_get_instance_private (o)

G_DEFINE_TYPE_WITH_PRIVATE (PpsOutlines, pps_outlines, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_MARKUP,
	PROP_LABEL,
	PROP_EXPAND,
	PROP_CHILDREN,
	PROP_LINK,
};

void
pps_outlines_set_markup (PpsOutlines *pps_outlines,
                         const gchar *markup)
{
	PpsOutlinesPrivate *priv = GET_PRIVATE (pps_outlines);

	priv->markup = g_strdup (markup);

	g_object_notify (G_OBJECT (pps_outlines), "markup");
}

void
pps_outlines_set_label (PpsOutlines *pps_outlines,
                        const gchar *label)
{
	PpsOutlinesPrivate *priv = GET_PRIVATE (pps_outlines);

	priv->label = g_strdup (label);

	g_object_notify (G_OBJECT (pps_outlines), "label");
}

static void
pps_outlines_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
	PpsOutlines *outlines = PPS_OUTLINES (object);
	PpsOutlinesPrivate *priv = GET_PRIVATE (outlines);

	switch (prop_id) {
	case PROP_MARKUP:
		pps_outlines_set_markup (outlines,
		                         g_value_get_string (value));
		break;
	case PROP_LABEL:
		pps_outlines_set_label (outlines,
		                        g_value_get_string (value));
		break;
	case PROP_EXPAND:
		priv->expand = g_value_get_boolean (value);
		break;
	case PROP_CHILDREN:
		priv->children = g_value_get_object (value);
		break;
	case PROP_LINK:
		priv->link = g_object_ref (g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * pps_outlines_get_link
 * @pps_outlines: A #PpsOutlines
 *
 * Returns: (transfer none) (nullable): The #PpsLink of the outlines
 */
PpsLink *
pps_outlines_get_link (PpsOutlines *pps_outlines)
{
	PpsOutlinesPrivate *priv = GET_PRIVATE (pps_outlines);
	g_return_val_if_fail (PPS_IS_OUTLINES (pps_outlines), NULL);

	return priv->link;
}

/**
 * pps_outlines_set_expand:
 * @pps_outlines: A #PpsOutlines
 * @expand: Whether the outlines should be expanded
 *
 * Sets the 'expand' property of the outlines.
 */
void
pps_outlines_set_expand (PpsOutlines *pps_outlines, gboolean expand)
{
	PpsOutlinesPrivate *priv = GET_PRIVATE (pps_outlines);
	g_return_if_fail (PPS_IS_OUTLINES (pps_outlines));

	priv->expand = expand;
	g_object_notify (G_OBJECT (pps_outlines), "expand");
}

/**
 * pps_outlines_get_expand:
 * @pps_outlines: A #PpsOutlines
 *
 *
 * Returns: Whether the outlines should be expanded
 */
gboolean
pps_outlines_get_expand (PpsOutlines *pps_outlines)
{
	g_return_val_if_fail (PPS_IS_OUTLINES (pps_outlines), FALSE);

	PpsOutlinesPrivate *priv = GET_PRIVATE (pps_outlines);
	return priv->expand;
}

/**
 * pps_outlines_set_children:
 * @pps_outlines: A #PpsOutlines
 * @children: (transfer full): The children of the outlines
 *
 * Sets the 'children' property of the outlines.
 */
void
pps_outlines_set_children (PpsOutlines *pps_outlines, GListModel *children)
{
	PpsOutlinesPrivate *priv = GET_PRIVATE (pps_outlines);
	g_return_if_fail (PPS_IS_OUTLINES (pps_outlines));

	if (g_set_object (&priv->children, children))
		g_object_notify (G_OBJECT (pps_outlines), "children");
}

/**
 * pps_outlines_get_children:
 * @pps_outlines: A #PpsOutlines
 *
 * Returns: (nullable) (transfer none): The children of the outlines
 */
GListModel *
pps_outlines_get_children (PpsOutlines *pps_outlines)
{
	PpsOutlinesPrivate *priv = GET_PRIVATE (pps_outlines);
	g_return_val_if_fail (PPS_IS_OUTLINES (pps_outlines), NULL);

	return priv->children;
}

static void
pps_outlines_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
	PpsOutlines *outlines = PPS_OUTLINES (object);
	PpsOutlinesPrivate *priv = GET_PRIVATE (outlines);

	switch (prop_id) {
	case PROP_MARKUP:
		g_value_set_string (value, priv->markup);
		break;
	case PROP_LABEL:
		g_value_set_string (value, priv->label);
		break;
	case PROP_EXPAND:
		g_value_set_boolean (value, priv->expand);
		break;
	case PROP_CHILDREN:
		g_value_set_object (value, priv->children);
		break;
	case PROP_LINK:
		g_value_set_object (value, priv->link);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
pps_outlines_dispose (GObject *object)
{
	PpsOutlines *outlines = PPS_OUTLINES (object);
	PpsOutlinesPrivate *priv = GET_PRIVATE (outlines);

	g_free (priv->markup);
	g_free (priv->label);
	g_clear_object (&priv->children);
	g_clear_object (&priv->link);

	G_OBJECT_CLASS (pps_outlines_parent_class)->dispose (object);
}

static void
pps_outlines_init (PpsOutlines *pps_outlines)
{
}

static void
pps_outlines_class_init (PpsOutlinesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = pps_outlines_set_property;
	object_class->get_property = pps_outlines_get_property;
	object_class->dispose = pps_outlines_dispose;

	g_object_class_install_property (object_class,
	                                 PROP_MARKUP,
	                                 g_param_spec_string ("markup",
	                                                      "markup",
	                                                      "The markup of the outlines",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label",
	                                                      "The label of the outlines",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_EXPAND,
	                                 g_param_spec_boolean ("expand",
	                                                       "expand",
	                                                       "Whether the outlines should be expanded",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE |
	                                                           G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_CHILDREN,
	                                 g_param_spec_object ("children",
	                                                      "Children",
	                                                      "The children of the outlines",
	                                                      G_TYPE_LIST_MODEL,
	                                                      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
	                                 PROP_LINK,
	                                 g_param_spec_object ("link",
	                                                      "Link",
	                                                      "The link of the outlines",
	                                                      PPS_TYPE_LINK,
	                                                      G_PARAM_READWRITE));
}
