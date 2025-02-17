/* pps-search-result.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2024 Markus Göllnitz  <camelcasenick@bewares.it>
 *
 * Papers is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Papers is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pps-search-result.h"
#include <string.h>

typedef struct
{
	gchar *markup;
	gchar *label;
	guint page;
	guint index;
	guint global_index;
	GList *find_rectangles;
} PpsSearchResultPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsSearchResult, pps_search_result, G_TYPE_OBJECT)

#define GET_PRIVATE(o) pps_search_result_get_instance_private (o)

static void
pps_search_result_init (PpsSearchResult *self)
{
}

static void
pps_search_result_dispose (GObject *object)
{
	PpsSearchResult *self = PPS_SEARCH_RESULT (object);
	PpsSearchResultPrivate *priv = GET_PRIVATE (self);

	g_clear_pointer (&priv->markup, g_free);
	g_clear_pointer (&priv->label, g_free);
	g_clear_list (&priv->find_rectangles, (GDestroyNotify) pps_find_rectangle_free);

	G_OBJECT_CLASS (pps_search_result_parent_class)->dispose (object);
}

static void
pps_search_result_class_init (PpsSearchResultClass *result_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (result_class);

	object_class->dispose = pps_search_result_dispose;
}

PpsSearchResult *
pps_search_result_new (gchar *markup,
                       gchar *label,
                       guint page,
                       guint index,
                       guint global_index,
                       PpsFindRectangle *rect)
{
	PpsSearchResult *result = g_object_new (PPS_TYPE_SEARCH_RESULT, NULL);
	PpsSearchResultPrivate *priv = GET_PRIVATE (result);

	priv->markup = g_strdup (markup);
	priv->label = g_strdup (label);
	priv->page = page;
	priv->index = index;
	priv->global_index = global_index;
	priv->find_rectangles = g_list_append (priv->find_rectangles,
	                                       pps_find_rectangle_copy (rect));

	return result;
}

const gchar *
pps_search_result_get_markup (PpsSearchResult *self)
{
	PpsSearchResultPrivate *priv = GET_PRIVATE (self);

	return priv->markup;
}

const gchar *
pps_search_result_get_label (PpsSearchResult *self)
{
	PpsSearchResultPrivate *priv = GET_PRIVATE (self);

	return priv->label;
}

guint
pps_search_result_get_page (PpsSearchResult *self)
{
	PpsSearchResultPrivate *priv = GET_PRIVATE (self);

	return priv->page;
}

guint
pps_search_result_get_index (PpsSearchResult *self)
{
	PpsSearchResultPrivate *priv = GET_PRIVATE (self);

	return priv->index;
}

/**
 * pps_search_result_get_global_index:
 * @self: the PpsSearchResult
 *
 * Returns: the index of this result relative the complete result model.
 *
 * Since: 48.0
 */
guint
pps_search_result_get_global_index (PpsSearchResult *self)
{
	PpsSearchResultPrivate *priv = GET_PRIVATE (self);

	return priv->global_index;
}

/**
 * pps_search_result_get_rectangle_list:
 * @self: the PpsSearchResult
 *
 * Returns: (nullable) (transfer none) (element-type PpsFindRectangle): the
 * list of rectangles for this result.
 */
GList *
pps_search_result_get_rectangle_list (PpsSearchResult *self)
{
	PpsSearchResultPrivate *priv = GET_PRIVATE (self);

	return priv->find_rectangles;
}

/**
 * pps_search_result_append_rectangle:
 * @self: the PpsSearchResult
 * @rect: the #PpsFindRectangle to append
 *
 * Appends a rectangle to the search result. This should not be used outside the
 * creation of the search result.
 */
void
pps_search_result_append_rectangle (PpsSearchResult *self, PpsFindRectangle *rect)
{
	PpsSearchResultPrivate *priv = GET_PRIVATE (self);

	priv->find_rectangles = g_list_append (priv->find_rectangles,
	                                       pps_find_rectangle_copy (rect));
}
