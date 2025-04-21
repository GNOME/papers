// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-transition-effect.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2007 Carlos Garnacho <carlos@imendio.com>
 */

#include <config.h>

#include "pps-transition-effect.h"

#include "pps-document-type-builtins.h"

typedef struct PpsTransitionEffectPrivate PpsTransitionEffectPrivate;

struct PpsTransitionEffectPrivate {
	PpsTransitionEffectType type;
	PpsTransitionEffectAlignment alignment;
	PpsTransitionEffectDirection direction;

	gint duration;
	gint angle;
	gdouble scale;
	gdouble duration_real;

	guint rectangular : 1;
};

enum {
	PROP_0,
	PROP_TYPE,
	PROP_ALIGNMENT,
	PROP_DIRECTION,
	PROP_DURATION,
	PROP_DURATION_REAL,
	PROP_ANGLE,
	PROP_SCALE,
	PROP_RECTANGULAR
};

G_DEFINE_TYPE_WITH_PRIVATE (PpsTransitionEffect, pps_transition_effect, G_TYPE_OBJECT)

static void
pps_transition_effect_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
	PpsTransitionEffectPrivate *priv;

	priv = pps_transition_effect_get_instance_private (PPS_TRANSITION_EFFECT (object));

	switch (prop_id) {
	case PROP_TYPE:
		priv->type = g_value_get_enum (value);
		break;
	case PROP_ALIGNMENT:
		priv->alignment = g_value_get_enum (value);
		break;
	case PROP_DIRECTION:
		priv->direction = g_value_get_enum (value);
		break;
	case PROP_DURATION:
		priv->duration = g_value_get_int (value);
		break;
	case PROP_DURATION_REAL:
		priv->duration_real = g_value_get_double (value);
		break;
	case PROP_ANGLE:
		priv->angle = g_value_get_int (value);
		break;
	case PROP_SCALE:
		priv->scale = g_value_get_double (value);
		break;
	case PROP_RECTANGULAR:
		priv->rectangular = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
pps_transition_effect_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
	PpsTransitionEffectPrivate *priv;

	priv = pps_transition_effect_get_instance_private (PPS_TRANSITION_EFFECT (object));

	switch (prop_id) {
	case PROP_TYPE:
		g_value_set_enum (value, priv->type);
		break;
	case PROP_ALIGNMENT:
		g_value_set_enum (value, priv->alignment);
		break;
	case PROP_DIRECTION:
		g_value_set_enum (value, priv->direction);
		break;
	case PROP_DURATION:
		g_value_set_int (value, priv->duration);
		break;
	case PROP_DURATION_REAL:
		g_value_set_double (value, priv->duration_real);
		break;
	case PROP_ANGLE:
		g_value_set_int (value, priv->angle);
		break;
	case PROP_SCALE:
		g_value_set_double (value, priv->scale);
		break;
	case PROP_RECTANGULAR:
		g_value_set_enum (value, priv->rectangular);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
pps_transition_effect_init (PpsTransitionEffect *effect)
{
	PpsTransitionEffectPrivate *priv;

	priv = pps_transition_effect_get_instance_private (effect);

	priv->scale = 1.;
}

static void
pps_transition_effect_class_init (PpsTransitionEffectClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = pps_transition_effect_set_property;
	object_class->get_property = pps_transition_effect_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_TYPE,
	                                 g_param_spec_enum ("type",
	                                                    "Effect type",
	                                                    "Page transition effect type",
	                                                    PPS_TYPE_TRANSITION_EFFECT_TYPE,
	                                                    PPS_TRANSITION_EFFECT_REPLACE,
	                                                    G_PARAM_READWRITE |
	                                                        G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_ALIGNMENT,
	                                 g_param_spec_enum ("alignment",
	                                                    "Effect alignment",
	                                                    "Alignment for the effect",
	                                                    PPS_TYPE_TRANSITION_EFFECT_ALIGNMENT,
	                                                    PPS_TRANSITION_ALIGNMENT_HORIZONTAL,
	                                                    G_PARAM_READWRITE |
	                                                        G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_DIRECTION,
	                                 g_param_spec_enum ("direction",
	                                                    "Effect direction",
	                                                    "Direction for the effect",
	                                                    PPS_TYPE_TRANSITION_EFFECT_DIRECTION,
	                                                    PPS_TRANSITION_DIRECTION_INWARD,
	                                                    G_PARAM_READWRITE |
	                                                        G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_DURATION,
	                                 g_param_spec_int ("duration",
	                                                   "Effect duration",
	                                                   "Effect duration in seconds",
	                                                   0, G_MAXINT, 0,
	                                                   G_PARAM_READWRITE |
	                                                       G_PARAM_STATIC_STRINGS |
	                                                       G_PARAM_DEPRECATED));
	g_object_class_install_property (object_class,
	                                 PROP_DURATION_REAL,
	                                 g_param_spec_double ("duration-real",
	                                                      "Effect duration in seconds (expressed as decimal number)",
	                                                      "Effect duration in seconds (expressed as decimal number)",
	                                                      0., 86400., 0., /* Arbitrary 1 day max value */
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_ANGLE,
	                                 g_param_spec_int ("angle",
	                                                   "Effect angle",
	                                                   "Effect angle in degrees, counted "
	                                                   "counterclockwise from left to right",
	                                                   0, 360, 0,
	                                                   G_PARAM_READWRITE |
	                                                       G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_SCALE,
	                                 g_param_spec_double ("scale",
	                                                      "Effect scale",
	                                                      "Scale at which the effect is applied",
	                                                      0., 1., 1.,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_RECTANGULAR,
	                                 g_param_spec_boolean ("rectangular",
	                                                       "Rectangular area",
	                                                       "Whether the covered area is rectangular",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE |
	                                                           G_PARAM_STATIC_STRINGS));
}

PpsTransitionEffect *
pps_transition_effect_new (PpsTransitionEffectType type,
                           const gchar *first_property_name,
                           ...)
{
	GObject *object;
	va_list args;

	object = g_object_new (PPS_TYPE_TRANSITION_EFFECT,
	                       "type", type,
	                       NULL);

	va_start (args, first_property_name);
	g_object_set_valist (object, first_property_name, args);
	va_end (args);

	return PPS_TRANSITION_EFFECT (object);
}
