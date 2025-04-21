// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2005 Red Hat, Inc.
 */

#include "pps-link.h"
#include <config.h>

enum {
	PROP_0,
	PROP_TITLE,
	PROP_ACTION
};

struct _PpsLink {
	GObject base_instance;
};

struct _PpsLinkPrivate {
	gchar *title;
	PpsLinkAction *action;
};

typedef struct _PpsLinkPrivate PpsLinkPrivate;
#define GET_PRIVATE(o) pps_link_get_instance_private (o)

G_DEFINE_TYPE_WITH_PRIVATE (PpsLink, pps_link, G_TYPE_OBJECT)

const gchar *
pps_link_get_title (PpsLink *self)
{
	g_return_val_if_fail (PPS_IS_LINK (self), NULL);
	PpsLinkPrivate *priv = GET_PRIVATE (self);

	return priv->title;
}

/**
 * pps_link_get_action:
 * @self: an #PpsLink
 *
 * Returns: (transfer none): an #PpsLinkAction
 */
PpsLinkAction *
pps_link_get_action (PpsLink *self)
{
	g_return_val_if_fail (PPS_IS_LINK (self), NULL);
	PpsLinkPrivate *priv = GET_PRIVATE (self);

	return priv->action;
}

static void
pps_link_get_property (GObject *object,
                       guint prop_id,
                       GValue *value,
                       GParamSpec *param_spec)
{
	PpsLink *self;

	self = PPS_LINK (object);
	PpsLinkPrivate *priv = GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_TITLE:
		g_value_set_string (value, priv->title);
		break;
	case PROP_ACTION:
		g_value_set_object (value, priv->action);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
		                                   prop_id,
		                                   param_spec);
		break;
	}
}

static void
pps_link_set_property (GObject *object,
                       guint prop_id,
                       const GValue *value,
                       GParamSpec *param_spec)
{
	PpsLink *self = PPS_LINK (object);
	PpsLinkPrivate *priv = GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_TITLE:
		priv->title = g_value_dup_string (value);
		break;
	case PROP_ACTION:
		priv->action = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
		                                   prop_id,
		                                   param_spec);
		break;
	}
}

static void
pps_link_finalize (GObject *object)
{
	PpsLinkPrivate *priv = GET_PRIVATE (PPS_LINK (object));

	g_clear_pointer (&priv->title, g_free);
	g_clear_object (&priv->action);

	G_OBJECT_CLASS (pps_link_parent_class)->finalize (object);
}

static void
pps_link_init (PpsLink *pps_link)
{
}

static void
pps_link_class_init (PpsLinkClass *pps_window_class)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS (pps_window_class);

	g_object_class->set_property = pps_link_set_property;
	g_object_class->get_property = pps_link_get_property;

	g_object_class->finalize = pps_link_finalize;

	g_object_class_install_property (g_object_class,
	                                 PROP_TITLE,
	                                 g_param_spec_string ("title",
	                                                      "Link Title",
	                                                      "The link title",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_ACTION,
	                                 g_param_spec_object ("action",
	                                                      "Link Action",
	                                                      "The link action",
	                                                      PPS_TYPE_LINK_ACTION,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
}

/**
 * pps_link_new:
 * @title: (nullable): the title
 * @action: a #PpsLinkAction
 */
PpsLink *
pps_link_new (const char *title,
              PpsLinkAction *action)
{
	return PPS_LINK (g_object_new (PPS_TYPE_LINK,
	                               "title", title,
	                               "action", action,
	                               NULL));
}
