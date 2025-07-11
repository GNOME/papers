// SPDX-License-Identifier: GPL-2.0-or-later
/* this file is part of papers, a gnome document viewer
 *
 *  Copyright (C) 2006 Carlos Garcia Campos <carlosgc@gnome.org>
 *  Copyright (C) 2005 Red Hat, Inc.
 */

#include <config.h>

#include "pps-document-type-builtins.h"
#include "pps-link-dest.h"

enum {
	PROP_0,
	PROP_TYPE,
	PROP_PAGE,
	PROP_LEFT,
	PROP_TOP,
	PROP_BOTTOM,
	PROP_RIGHT,
	PROP_ZOOM,
	PROP_CHANGE,
	PROP_NAMED,
	PROP_PAGE_LABEL
};

typedef enum {
	PPS_DEST_CHANGE_TOP = 1 << 0,
	PPS_DEST_CHANGE_LEFT = 1 << 1,
	PPS_DEST_CHANGE_ZOOM = 1 << 2
} PpsDestChange;

struct _PpsLinkDest {
	GObject base_instance;
};

struct _PpsLinkDestPrivate {
	PpsLinkDestType type;
	int page;
	double top;
	double left;
	double bottom;
	double right;
	double zoom;
	PpsDestChange change;
	gchar *named;
	gchar *page_label;
};

typedef struct _PpsLinkDestPrivate PpsLinkDestPrivate;

#define GET_PRIVATE(o) pps_link_dest_get_instance_private (o)

G_DEFINE_TYPE_WITH_PRIVATE (PpsLinkDest, pps_link_dest, G_TYPE_OBJECT)

PpsLinkDestType
pps_link_dest_get_dest_type (PpsLinkDest *self)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (self), 0);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	return priv->type;
}

gint
pps_link_dest_get_page (PpsLinkDest *self)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (self), -1);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	return priv->page;
}

/**
 * pps_link_dest_get_top: (get-property top)
 * @self: a `PpsLinkDest`
 * @change_top: (out caller-allocates): whether the `PPS_DEST_CHANGE_TOP` flag is set
 */
gdouble
pps_link_dest_get_top (PpsLinkDest *self,
                       gboolean *change_top)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (self), 0);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	if (change_top)
		*change_top = (priv->change & PPS_DEST_CHANGE_TOP);

	return priv->top;
}

/**
 * pps_link_dest_get_left: (get-property left)
 * @self: a `PpsLinkDest`
 * @change_left: (out caller-allocates): whether the `PPS_DEST_CHANGE_LEFT` flag is set
 */
gdouble
pps_link_dest_get_left (PpsLinkDest *self,
                        gboolean *change_left)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (self), 0);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	if (change_left)
		*change_left = (priv->change & PPS_DEST_CHANGE_LEFT);

	return priv->left;
}

gdouble
pps_link_dest_get_bottom (PpsLinkDest *self)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (self), 0);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	return priv->bottom;
}

gdouble
pps_link_dest_get_right (PpsLinkDest *self)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (self), 0);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	return priv->right;
}

/**
 * pps_link_dest_get_zoom: (get-property zoom)
 * @self: a `PpsLinkDest`
 * @change_zoom: (out caller-allocates): whether the `PPS_DEST_CHANGE_ZOOM` flag is set
 */
gdouble
pps_link_dest_get_zoom (PpsLinkDest *self,
                        gboolean *change_zoom)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (self), 0);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	if (change_zoom)
		*change_zoom = (priv->change & PPS_DEST_CHANGE_ZOOM);

	return priv->zoom;
}

const gchar *
pps_link_dest_get_named_dest (PpsLinkDest *self)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (self), NULL);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	return priv->named;
}

const gchar *
pps_link_dest_get_page_label (PpsLinkDest *self)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (self), NULL);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	return priv->page_label;
}

static void
pps_link_dest_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *param_spec)
{
	PpsLinkDest *self = PPS_LINK_DEST (object);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_TYPE:
		g_value_set_enum (value, priv->type);
		break;
	case PROP_PAGE:
		g_value_set_int (value, priv->page);
		break;
	case PROP_TOP:
		g_value_set_double (value, priv->top);
		break;
	case PROP_LEFT:
		g_value_set_double (value, priv->left);
		break;
	case PROP_BOTTOM:
		g_value_set_double (value, priv->bottom);
		break;
	case PROP_RIGHT:
		g_value_set_double (value, priv->left);
		break;
	case PROP_ZOOM:
		g_value_set_double (value, priv->zoom);
		break;
	case PROP_CHANGE:
		g_value_set_uint (value, priv->change);
		break;
	case PROP_NAMED:
		g_value_set_string (value, priv->named);
		break;
	case PROP_PAGE_LABEL:
		g_value_set_string (value, priv->page_label);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
		                                   prop_id,
		                                   param_spec);
		break;
	}
}

static void
pps_link_dest_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *param_spec)
{
	PpsLinkDest *self = PPS_LINK_DEST (object);
	PpsLinkDestPrivate *priv = GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_TYPE:
		priv->type = g_value_get_enum (value);
		break;
	case PROP_PAGE:
		priv->page = g_value_get_int (value);
		break;
	case PROP_TOP:
		priv->top = g_value_get_double (value);
		break;
	case PROP_LEFT:
		priv->left = g_value_get_double (value);
		break;
	case PROP_BOTTOM:
		priv->bottom = g_value_get_double (value);
		break;
	case PROP_RIGHT:
		priv->right = g_value_get_double (value);
		break;
	case PROP_ZOOM:
		priv->zoom = g_value_get_double (value);
		break;
	case PROP_CHANGE:
		priv->change = g_value_get_uint (value);
		break;
	case PROP_NAMED:
		priv->named = g_value_dup_string (value);
		break;
	case PROP_PAGE_LABEL:
		priv->page_label = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
		                                   prop_id,
		                                   param_spec);
		break;
	}
}

static void
pps_link_dest_finalize (GObject *object)
{
	PpsLinkDestPrivate *priv = GET_PRIVATE (PPS_LINK_DEST (object));

	g_clear_pointer (&priv->named, g_free);
	g_clear_pointer (&priv->page_label, g_free);

	G_OBJECT_CLASS (pps_link_dest_parent_class)->finalize (object);
}

static void
pps_link_dest_init (PpsLinkDest *pps_link_dest)
{
}

static void
pps_link_dest_class_init (PpsLinkDestClass *pps_link_dest_class)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS (pps_link_dest_class);

	g_object_class->set_property = pps_link_dest_set_property;
	g_object_class->get_property = pps_link_dest_get_property;

	g_object_class->finalize = pps_link_dest_finalize;

	g_object_class_install_property (g_object_class,
	                                 PROP_TYPE,
	                                 g_param_spec_enum ("type",
	                                                    "Dest Type",
	                                                    "The destination type",
	                                                    PPS_TYPE_LINK_DEST_TYPE,
	                                                    PPS_LINK_DEST_TYPE_UNKNOWN,
	                                                    G_PARAM_READWRITE |
	                                                        G_PARAM_CONSTRUCT_ONLY |
	                                                        G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_PAGE,
	                                 g_param_spec_int ("page",
	                                                   "Dest Page",
	                                                   "The destination page",
	                                                   -1,
	                                                   G_MAXINT,
	                                                   0,
	                                                   G_PARAM_READWRITE |
	                                                       G_PARAM_CONSTRUCT_ONLY |
	                                                       G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_LEFT,
	                                 g_param_spec_double ("left",
	                                                      "Left coordinate",
	                                                      "The left coordinate",
	                                                      -G_MAXDOUBLE,
	                                                      G_MAXDOUBLE,
	                                                      0,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_TOP,
	                                 g_param_spec_double ("top",
	                                                      "Top coordinate",
	                                                      "The top coordinate",
	                                                      -G_MAXDOUBLE,
	                                                      G_MAXDOUBLE,
	                                                      0,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_BOTTOM,
	                                 g_param_spec_double ("bottom",
	                                                      "Bottom coordinate",
	                                                      "The bottom coordinate",
	                                                      -G_MAXDOUBLE,
	                                                      G_MAXDOUBLE,
	                                                      0,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_RIGHT,
	                                 g_param_spec_double ("right",
	                                                      "Right coordinate",
	                                                      "The right coordinate",
	                                                      -G_MAXDOUBLE,
	                                                      G_MAXDOUBLE,
	                                                      0,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (g_object_class,
	                                 PROP_ZOOM,
	                                 g_param_spec_double ("zoom",
	                                                      "Zoom",
	                                                      "Zoom",
	                                                      0,
	                                                      G_MAXDOUBLE,
	                                                      0,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_CHANGE,
	                                 g_param_spec_uint ("change",
	                                                    "Change",
	                                                    "Wether top, left, and zoom should be changed",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    0,
	                                                    G_PARAM_READWRITE |
	                                                        G_PARAM_CONSTRUCT_ONLY |
	                                                        G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_NAMED,
	                                 g_param_spec_string ("named",
	                                                      "Named destination",
	                                                      "The named destination",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (g_object_class,
	                                 PROP_PAGE_LABEL,
	                                 g_param_spec_string ("page-label",
	                                                      "Label of the page",
	                                                      "The label of the destination page",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
}

PpsLinkDest *
pps_link_dest_new_page (gint page)
{
	return PPS_LINK_DEST (g_object_new (PPS_TYPE_LINK_DEST,
	                                    "page", page,
	                                    "type", PPS_LINK_DEST_TYPE_PAGE,
	                                    NULL));
}

PpsLinkDest *
pps_link_dest_new_xyz (gint page,
                       gdouble left,
                       gdouble top,
                       gdouble zoom,
                       gboolean change_left,
                       gboolean change_top,
                       gboolean change_zoom)
{
	PpsDestChange change = 0;

	if (change_left)
		change |= PPS_DEST_CHANGE_LEFT;
	if (change_top)
		change |= PPS_DEST_CHANGE_TOP;
	if (change_zoom)
		change |= PPS_DEST_CHANGE_ZOOM;

	return PPS_LINK_DEST (g_object_new (PPS_TYPE_LINK_DEST,
	                                    "page", page,
	                                    "type", PPS_LINK_DEST_TYPE_XYZ,
	                                    "left", left,
	                                    "top", top,
	                                    "zoom", zoom,
	                                    "change", change,
	                                    NULL));
}

PpsLinkDest *
pps_link_dest_new_fit (gint page)
{
	return PPS_LINK_DEST (g_object_new (PPS_TYPE_LINK_DEST,
	                                    "page", page,
	                                    "type", PPS_LINK_DEST_TYPE_FIT,
	                                    NULL));
}

PpsLinkDest *
pps_link_dest_new_fith (gint page,
                        gdouble top,
                        gboolean change_top)
{
	PpsDestChange change = 0;

	if (change_top)
		change |= PPS_DEST_CHANGE_TOP;

	return PPS_LINK_DEST (g_object_new (PPS_TYPE_LINK_DEST,
	                                    "page", page,
	                                    "type", PPS_LINK_DEST_TYPE_FITH,
	                                    "top", top,
	                                    "change", change,
	                                    NULL));
}

PpsLinkDest *
pps_link_dest_new_fitv (gint page,
                        gdouble left,
                        gboolean change_left)
{
	PpsDestChange change = 0;

	if (change_left)
		change |= PPS_DEST_CHANGE_LEFT;

	return PPS_LINK_DEST (g_object_new (PPS_TYPE_LINK_DEST,
	                                    "page", page,
	                                    "type", PPS_LINK_DEST_TYPE_FITV,
	                                    "left", left,
	                                    "change", change,
	                                    NULL));
}

PpsLinkDest *
pps_link_dest_new_fitr (gint page,
                        gdouble left,
                        gdouble bottom,
                        gdouble right,
                        gdouble top)
{
	PpsDestChange change = PPS_DEST_CHANGE_TOP | PPS_DEST_CHANGE_LEFT;

	return PPS_LINK_DEST (g_object_new (PPS_TYPE_LINK_DEST,
	                                    "page", page,
	                                    "type", PPS_LINK_DEST_TYPE_FITR,
	                                    "left", left,
	                                    "bottom", bottom,
	                                    "right", right,
	                                    "top", top,
	                                    "change", change,
	                                    NULL));
}

PpsLinkDest *
pps_link_dest_new_named (const gchar *named_dest)
{
	return PPS_LINK_DEST (g_object_new (PPS_TYPE_LINK_DEST,
	                                    "named", named_dest,
	                                    "type", PPS_LINK_DEST_TYPE_NAMED,
	                                    NULL));
}

PpsLinkDest *
pps_link_dest_new_page_label (const gchar *page_label)
{
	return PPS_LINK_DEST (g_object_new (PPS_TYPE_LINK_DEST,
	                                    "page_label", page_label,
	                                    "type", PPS_LINK_DEST_TYPE_PAGE_LABEL,
	                                    NULL));
}

/**
 * pps_link_dest_equal:
 * @a: a #PpsLinkDest
 * @b: a #PpsLinkDest
 *
 * Checks whether @a and @b are equal.
 *
 * Returns: %TRUE iff @a and @b are equal
 */
gboolean
pps_link_dest_equal (PpsLinkDest *a,
                     PpsLinkDest *b)
{
	g_return_val_if_fail (PPS_IS_LINK_DEST (a), FALSE);
	g_return_val_if_fail (PPS_IS_LINK_DEST (b), FALSE);

	PpsLinkDestPrivate *a_priv = GET_PRIVATE (a);
	PpsLinkDestPrivate *b_priv = GET_PRIVATE (b);

	if (a == b)
		return TRUE;

	if (a_priv->type != b_priv->type)
		return FALSE;

	switch (a_priv->type) {
	case PPS_LINK_DEST_TYPE_PAGE:
		return a_priv->page == b_priv->page;

	case PPS_LINK_DEST_TYPE_XYZ:
		return a_priv->page == b_priv->page &&
		       a_priv->left == b_priv->left &&
		       a_priv->top == b_priv->top &&
		       a_priv->zoom == b_priv->zoom &&
		       a_priv->change == b_priv->change;

	case PPS_LINK_DEST_TYPE_FIT:
		return a_priv->page == b_priv->page;

	case PPS_LINK_DEST_TYPE_FITH:
		return a_priv->page == b_priv->page &&
		       a_priv->top == b_priv->top &&
		       a_priv->change == b_priv->change;

	case PPS_LINK_DEST_TYPE_FITV:
		return a_priv->page == b_priv->page &&
		       a_priv->left == b_priv->left &&
		       a_priv->change == b_priv->change;

	case PPS_LINK_DEST_TYPE_FITR:
		return a_priv->page == b_priv->page &&
		       a_priv->left == b_priv->left &&
		       a_priv->top == b_priv->top &&
		       a_priv->right == b_priv->right &&
		       a_priv->bottom == b_priv->bottom &&
		       a_priv->change == b_priv->change;

	case PPS_LINK_DEST_TYPE_NAMED:
		return !g_strcmp0 (a_priv->named, b_priv->named);

	case PPS_LINK_DEST_TYPE_PAGE_LABEL:
		return !g_strcmp0 (a_priv->page_label, b_priv->page_label);

	default:
		return FALSE;
	}

	return FALSE;
}
