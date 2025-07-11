// SPDX-License-Identifier: GPL-2.0-or-later
/* pps-mapping.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
 */

#include "pps-mapping-list.h"

/**
 * SECTION: pps-mapping-list
 * @short_description: a refcounted list of #PpsMappings.
 */
struct _PpsMappingList {
	guint page;
	GList *list;
	GDestroyNotify data_destroy_func;
	volatile gint ref_count;
};

G_DEFINE_BOXED_TYPE (PpsMappingList, pps_mapping_list, pps_mapping_list_ref, pps_mapping_list_unref)

/**
 * pps_mapping_list_find:
 * @mapping_list: an #PpsMappingList
 * @data: mapping data to find
 *
 * Returns: (transfer none): an #PpsMapping
 */
PpsMapping *
pps_mapping_list_find (PpsMappingList *mapping_list,
                       gconstpointer data)
{
	GList *list;

	for (list = mapping_list->list; list; list = list->next) {
		PpsMapping *mapping = list->data;

		if (mapping->data == data)
			return mapping;
	}

	return NULL;
}

/**
 * pps_mapping_list_nth:
 * @mapping_list: an #PpsMappingList
 * @n: the position to retrieve
 *
 * Returns: (transfer none): the #Ppsmapping at position @n in @mapping_list
 */
PpsMapping *
pps_mapping_list_nth (PpsMappingList *mapping_list,
                      guint n)
{
	g_return_val_if_fail (mapping_list != NULL, NULL);

	return (PpsMapping *) g_list_nth_data (mapping_list->list, n);
}

static int
cmp_mapping_area_size (PpsMapping *a,
                       PpsMapping *b)
{
	gdouble wa, ha, wb, hb;

	wa = a->area.x2 - a->area.x1;
	ha = a->area.y2 - a->area.y1;
	wb = b->area.x2 - b->area.x1;
	hb = b->area.y2 - b->area.y1;

	if (wa == wb) {
		if (ha == hb)
			return 0;
		return (ha < hb) ? -1 : 1;
	}

	if (ha == hb) {
		return (wa < wb) ? -1 : 1;
	}

	return (wa * ha < wb * hb) ? -1 : 1;
}

/**
 * pps_mapping_list_get:
 * @mapping_list: an #PpsMappingList
 * @point: coordinate
 *
 * Returns: (transfer none): the #PpsMapping in the list at coordinates (x, y)
 */
PpsMapping *
pps_mapping_list_get (PpsMappingList *mapping_list,
                      const PpsPoint *point)
{
	GList *list;
	PpsMapping *found = NULL;

	g_return_val_if_fail (mapping_list != NULL, NULL);

	for (list = mapping_list->list; list; list = list->next) {
		PpsMapping *mapping = list->data;

		if ((point->x >= mapping->area.x1) &&
		    (point->y >= mapping->area.y1) &&
		    (point->x <= mapping->area.x2) &&
		    (point->y <= mapping->area.y2)) {

			/* In case of only one match choose that. Otherwise
			 * compare the area of the bounding boxes and return the
			 * smallest element */
			if (found == NULL || cmp_mapping_area_size (mapping, found) < 0)
				found = mapping;
		}
	}

	return found;
}

/**
 * pps_mapping_list_get_data:
 * @mapping_list: an #PpsMappingList
 * @point: coordinate
 *
 * Returns: (transfer none): the data of a mapping in the list at coordinates (x, y)
 */
gpointer
pps_mapping_list_get_data (PpsMappingList *mapping_list,
                           const PpsPoint *point)
{
	PpsMapping *mapping;

	mapping = pps_mapping_list_get (mapping_list, point);
	if (mapping)
		return mapping->data;

	return NULL;
}

/**
 * pps_mapping_list_get_list:
 * @mapping_list: an #PpsMappingList
 *
 * Returns: (transfer none) (element-type PpsMapping): the data for this mapping list
 */
GList *
pps_mapping_list_get_list (PpsMappingList *mapping_list)
{
	return mapping_list ? mapping_list->list : NULL;
}

/**
 * pps_mapping_list_remove:
 * @mapping_list: an #PpsMappingList
 * @mapping: #PpsMapping to remove
 */
void
pps_mapping_list_remove (PpsMappingList *mapping_list,
                         PpsMapping *mapping)
{
	mapping_list->list = g_list_remove (mapping_list->list, mapping);
	mapping_list->data_destroy_func (mapping->data);
	g_free (mapping);
}

guint
pps_mapping_list_get_page (PpsMappingList *mapping_list)
{
	return mapping_list->page;
}

guint
pps_mapping_list_length (PpsMappingList *mapping_list)
{
	g_return_val_if_fail (mapping_list != NULL, 0);

	return g_list_length (mapping_list->list);
}

/**
 * pps_mapping_list_new:
 * @page: page index for this mapping
 * @list: (element-type PpsMapping): a #GList of data for the page
 * @data_destroy_func: function to free a list element
 *
 * Returns: an #PpsMappingList
 */
PpsMappingList *
pps_mapping_list_new (guint page,
                      GList *list,
                      GDestroyNotify data_destroy_func)
{
	PpsMappingList *mapping_list;

	g_return_val_if_fail (data_destroy_func != NULL, NULL);

	mapping_list = g_slice_new (PpsMappingList);
	mapping_list->page = page;
	mapping_list->list = list;
	mapping_list->data_destroy_func = data_destroy_func;
	mapping_list->ref_count = 1;

	return mapping_list;
}

PpsMappingList *
pps_mapping_list_ref (PpsMappingList *mapping_list)
{
	g_return_val_if_fail (mapping_list != NULL, NULL);
	g_return_val_if_fail (mapping_list->ref_count > 0, mapping_list);

	g_atomic_int_add (&mapping_list->ref_count, 1);

	return mapping_list;
}

static void
mapping_list_free_foreach (PpsMapping *mapping,
                           GDestroyNotify destroy_func)
{
	destroy_func (mapping->data);
	g_free (mapping);
}

void
pps_mapping_list_unref (PpsMappingList *mapping_list)
{
	g_return_if_fail (mapping_list != NULL);
	g_return_if_fail (mapping_list->ref_count > 0);

	if (g_atomic_int_add (&mapping_list->ref_count, -1) - 1 == 0) {
		g_list_foreach (mapping_list->list,
		                (GFunc) mapping_list_free_foreach,
		                mapping_list->data_destroy_func);
		g_list_free (mapping_list->list);
		g_slice_free (PpsMappingList, mapping_list);
	}
}
