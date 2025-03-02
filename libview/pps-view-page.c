/* pps-view-page.c
 *  this file is part of papers, a gnome document viewer
 *
 * Copyright (C) 2025 Markus Göllnitz <camelcasenick@bewares.it>
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

#include "pps-view-page.h"

#include <string.h>

#include "pps-colors.h"
#include "pps-debug.h"

#define PPS_STYLE_CLASS_DOCUMENT_PAGE "document-page"
#define PPS_STYLE_CLASS_INVERTED "inverted"

typedef struct
{
	gint index;

	PpsDocumentModel *model;
	PpsSearchContext *search_context;

	PpsPageCache *page_cache;
	PpsPixbufCache *pixbuf_cache;

	gboolean had_search_results;
} PpsViewPagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PpsViewPage, pps_view_page, GTK_TYPE_WIDGET)

#define GET_PRIVATE(o) pps_view_page_get_instance_private (o)

static void
pps_view_page_measure (GtkWidget *widget,
                       GtkOrientation orientation,
                       int for_size,
                       int *minimum,
                       int *natural,
                       int *minimum_baseline,
                       int *natural_baseline)
{
	PpsViewPage *page = PPS_VIEW_PAGE (widget);
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	gint width = 0, height = 0;
	gdouble width_raw, height_raw;
	PpsDocument *document;
	gdouble scale;
	gint rotation;

	if (priv->model != NULL && pps_document_model_get_document (priv->model) != NULL && priv->index >= 0) {
		document = pps_document_model_get_document (priv->model);
		scale = pps_document_model_get_scale (priv->model);
		rotation = pps_document_model_get_rotation (priv->model);

		pps_document_get_page_size (document, priv->index, &width_raw, &height_raw);

		width = (gint) (((rotation == 0 || rotation == 180) ? width_raw : height_raw) * scale + 0.5);
		height = (gint) (((rotation == 0 || rotation == 180) ? height_raw : width_raw) * scale + 0.5);
	}

	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		*minimum = 0;
		*natural = width;
	} else {
		*minimum = 0;
		*natural = height;
	}
}

static void
doc_rect_to_view_rect (PpsViewPage *page,
                       const PpsRectangle *doc_rect,
                       GdkRectangle *view_rect)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	gdouble scale = pps_document_model_get_scale (priv->model);
	gdouble page_width = gtk_widget_get_width (GTK_WIDGET (page));
	gdouble page_height = gtk_widget_get_height (GTK_WIDGET (page));
	gdouble x, y, width, height;

	switch (pps_document_model_get_rotation (priv->model)) {
	case 0:
		x = doc_rect->x1 * scale;
		y = doc_rect->y1 * scale;
		width = (doc_rect->x2 - doc_rect->x1) * scale;
		height = (doc_rect->y2 - doc_rect->y1) * scale;

		break;
	case 90: {
		x = page_width - doc_rect->y2 * scale;
		y = doc_rect->x1 * scale;
		width = (doc_rect->y2 - doc_rect->y1) * scale;
		height = (doc_rect->x2 - doc_rect->x1) * scale;
	} break;
	case 180: {
		x = page_width - doc_rect->x2 * scale;
		y = page_height - doc_rect->y2 * scale;
		width = (doc_rect->x2 - doc_rect->x1) * scale;
		height = (doc_rect->y2 - doc_rect->y1) * scale;
	} break;
	case 270: {
		x = doc_rect->y1 * scale;
		y = page_height - doc_rect->x2 * scale;
		width = (doc_rect->y2 - doc_rect->y1) * scale;
		height = (doc_rect->x2 - doc_rect->x1) * scale;
	} break;
	default:
		g_assert_not_reached ();
	}

	view_rect->x = (gint) (x + 0.5);
	view_rect->y = (gint) (y + 0.5);
	view_rect->width = (gint) (width + 0.5);
	view_rect->height = (gint) (height + 0.5);
}

static PpsViewSelection *
find_selection_for_page (PpsViewPage *page)
{
	GList *list;
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	for (list = pps_pixbuf_cache_get_selection_list (priv->pixbuf_cache); list != NULL; list = list->next) {
		PpsViewSelection *selection;

		selection = (PpsViewSelection *) list->data;

		if (selection->page == priv->index)
			return selection;
	}

	return NULL;
}

static void
stroke_view_rect (GtkSnapshot *snapshot,
                  GdkRectangle *clip,
                  GdkRGBA *color,
                  GdkRectangle *view_rect)
{
	GdkRectangle intersect;
	GdkRGBA border_color[4] = { *color, *color, *color, *color };
	float border_width[4] = { 1, 1, 1, 1 };

	if (gdk_rectangle_intersect (view_rect, clip, &intersect)) {
		gtk_snapshot_append_border (snapshot,
		                            &GSK_ROUNDED_RECT_INIT (intersect.x, intersect.y,
		                                                    intersect.width, intersect.height),
		                            border_width, border_color);
	}
}

static void
stroke_doc_rect (GtkSnapshot *snapshot,
                 PpsViewPage *page,
                 GdkRGBA *color,
                 GdkRectangle *clip,
                 PpsRectangle *doc_rect)
{
	GdkRectangle view_rect;

	doc_rect_to_view_rect (page, doc_rect, &view_rect);
	stroke_view_rect (snapshot, clip, color, &view_rect);
}

static void
show_chars_border (GtkSnapshot *snapshot,
                   PpsViewPage *page,
                   GdkRectangle *clip)
{
	PpsRectangle *areas = NULL;
	guint n_areas = 0;
	guint i;
	GdkRGBA color = { 1, 0, 0, 1 };
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	pps_page_cache_get_text_layout (priv->page_cache, priv->index, &areas, &n_areas);
	if (!areas)
		return;

	for (i = 0; i < n_areas; i++) {
		PpsRectangle *doc_rect = areas + i;

		stroke_doc_rect (snapshot, page, &color, clip, doc_rect);
	}
}

static void
show_mapping_list_border (GtkSnapshot *snapshot,
                          PpsViewPage *page,
                          GdkRGBA *color,
                          GdkRectangle *clip,
                          PpsMappingList *mapping_list)
{
	GList *l;

	for (l = pps_mapping_list_get_list (mapping_list); l; l = g_list_next (l)) {
		PpsMapping *mapping = (PpsMapping *) l->data;

		stroke_doc_rect (snapshot, page, color, clip, &mapping->area);
	}
}

static void
show_links_border (GtkSnapshot *snapshot,
                   PpsViewPage *page,
                   GdkRectangle *clip)
{
	GdkRGBA color = { 0, 0, 1, 1 };
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	show_mapping_list_border (snapshot, page, &color, clip,
	                          pps_page_cache_get_link_mapping (priv->page_cache, priv->index));
}

static void
show_forms_border (GtkSnapshot *snapshot,
                   PpsViewPage *page,
                   GdkRectangle *clip)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GdkRGBA color = { 0, 1, 0, 1 };
	show_mapping_list_border (snapshot, page, &color, clip,
	                          pps_page_cache_get_form_field_mapping (priv->page_cache, priv->index));
}

static void
show_annots_border (GtkSnapshot *snapshot,
                    PpsViewPage *page,
                    GdkRectangle *clip)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GdkRGBA color = { 0, 1, 1, 1 };
	show_mapping_list_border (snapshot, page, &color, clip,
	                          pps_page_cache_get_annot_mapping (priv->page_cache, priv->index));
}

static void
show_images_border (GtkSnapshot *snapshot,
                    PpsViewPage *page,
                    GdkRectangle *clip)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GdkRGBA color = { 1, 0, 1, 1 };
	show_mapping_list_border (snapshot, page, &color, clip,
	                          pps_page_cache_get_image_mapping (priv->page_cache, priv->index));
}

static void
show_media_border (GtkSnapshot *snapshot,
                   PpsViewPage *page,
                   GdkRectangle *clip)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GdkRGBA color = { 1, 1, 0, 1 };
	show_mapping_list_border (snapshot, page, &color, clip,
	                          pps_page_cache_get_media_mapping (priv->page_cache, priv->index));
}

static void
show_selections_border (GtkSnapshot *snapshot,
                        PpsViewPage *page,
                        GdkRectangle *clip)
{
	cairo_region_t *region;
	guint i, n_rects;
	GdkRGBA color = { 0.75, 0.50, 0.25, 1 };
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	region = pps_page_cache_get_text_mapping (priv->page_cache, priv->index);
	if (!region)
		return;

	region = cairo_region_copy (region);
	n_rects = cairo_region_num_rectangles (region);
	for (i = 0; i < n_rects; i++) {
		GdkRectangle doc_rect_int;
		PpsRectangle doc_rect_float;

		cairo_region_get_rectangle (region, i, &doc_rect_int);

		/* Convert the doc rect to a PpsRectangle */
		doc_rect_float.x1 = doc_rect_int.x;
		doc_rect_float.y1 = doc_rect_int.y;
		doc_rect_float.x2 = doc_rect_int.x + doc_rect_int.width;
		doc_rect_float.y2 = doc_rect_int.y + doc_rect_int.height;

		stroke_doc_rect (snapshot, page, &color, clip, &doc_rect_float);
	}
	cairo_region_destroy (region);
}

static void
draw_debug_borders (GtkSnapshot *snapshot,
                    PpsViewPage *page,
                    GdkRectangle *clip)
{
	PpsDebugBorders borders = pps_debug_get_debug_borders ();

	if (borders & PPS_DEBUG_BORDER_CHARS)
		show_chars_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_LINKS)
		show_links_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_FORMS)
		show_forms_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_ANNOTS)
		show_annots_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_IMAGES)
		show_images_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_MEDIA)
		show_media_border (snapshot, page, clip);
	if (borders & PPS_DEBUG_BORDER_SELECTIONS)
		show_selections_border (snapshot, page, clip);
}

static void
draw_rect (GtkSnapshot *snapshot,
           const GdkRectangle *rect,
           const GdkRGBA *color)
{
	graphene_rect_t graphene_rect = GRAPHENE_RECT_INIT (rect->x,
	                                                    rect->y,
	                                                    rect->width,
	                                                    rect->height);

	gtk_snapshot_append_color (snapshot, color, &graphene_rect);
}

static void
draw_selection_region (GtkSnapshot *snapshot,
                       cairo_region_t *region,
                       GdkRGBA *color)
{
	cairo_rectangle_int_t box;
	gint n_boxes, i;

	n_boxes = cairo_region_num_rectangles (region);

	for (i = 0; i < n_boxes; i++) {
		cairo_region_get_rectangle (region, i, &box);
		draw_rect (snapshot, &box, color);
	}
}

static void
highlight_find_results (GtkSnapshot *snapshot,
                        PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	GtkSingleSelection *model = pps_search_context_get_result_model (priv->search_context);
	g_autoptr (GPtrArray) results = pps_search_context_get_results_on_page (priv->search_context, priv->index);
	PpsRectangle pps_rect;
	GdkRGBA color_selected, color_default;

	get_accent_color (&color_selected, NULL);
	color_selected.alpha *= 0.6;
	get_accent_color (&color_default, NULL);
	color_default.alpha *= 0.3;

	for (gint i = 0; i < results->len; i++) {
		PpsSearchResult *result = g_ptr_array_index (results, i);
		PpsFindRectangle *find_rect;
		GList *rectangles;
		GdkRectangle view_rectangle;
		GdkRGBA color = color_default;

		rectangles = pps_search_result_get_rectangle_list (result);
		find_rect = (PpsFindRectangle *) rectangles->data;
		pps_rect.x1 = find_rect->x1;
		pps_rect.x2 = find_rect->x2;
		pps_rect.y1 = find_rect->y1;
		pps_rect.y2 = find_rect->y2;

		if (result == gtk_single_selection_get_selected_item (model))
			color = color_selected;

		doc_rect_to_view_rect (page, &pps_rect, &view_rectangle);
		draw_rect (snapshot, &view_rectangle, &color);

		if (rectangles->next) {
			/* Draw now next result (which is second part of multi-line match) */
			find_rect = (PpsFindRectangle *) rectangles->next->data;
			pps_rect.x1 = find_rect->x1;
			pps_rect.x2 = find_rect->x2;
			pps_rect.y1 = find_rect->y1;
			pps_rect.y2 = find_rect->y2;
			doc_rect_to_view_rect (page, &pps_rect, &view_rectangle);
			draw_rect (snapshot, &view_rectangle, &color);
		}
	}
}

static void
draw_surface (GtkSnapshot *snapshot,
              GdkTexture *texture,
              const graphene_rect_t *area,
              gboolean inverted)
{
	gtk_snapshot_save (snapshot);

	if (inverted) {
		gtk_snapshot_push_blend (snapshot, GSK_BLEND_MODE_COLOR);
		gtk_snapshot_push_blend (snapshot, GSK_BLEND_MODE_DIFFERENCE);
		gtk_snapshot_append_color (snapshot, &(GdkRGBA) { 1., 1., 1., 1. }, area);
		gtk_snapshot_pop (snapshot);
		gtk_snapshot_append_texture (snapshot, texture, area);
		gtk_snapshot_pop (snapshot);
		gtk_snapshot_pop (snapshot);
		gtk_snapshot_append_texture (snapshot, texture, area);
		gtk_snapshot_pop (snapshot);
	} else {
		gtk_snapshot_append_texture (snapshot, texture, area);
	}

	gtk_snapshot_restore (snapshot);
}

static void
pps_view_page_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
	PpsViewPage *page = PPS_VIEW_PAGE (widget);
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	guint width = gtk_widget_get_width (widget);
	guint height = gtk_widget_get_height (widget);
	GdkTexture *page_texture = NULL, *selection_texture = NULL;
	GdkRectangle area_rect;
	graphene_rect_t area;
	cairo_region_t *region = NULL;
	gboolean inverted;
	gdouble scale;

	if (priv->model == NULL || priv->index < 0)
		return;

	inverted = pps_document_model_get_inverted_colors (priv->model);
	scale = pps_document_model_get_scale (priv->model);
	page_texture = pps_pixbuf_cache_get_texture (priv->pixbuf_cache, priv->index);

	if (!page_texture)
		return;

	area_rect.x = 0;
	area_rect.y = 0;
	area_rect.width = width;
	area_rect.height = height;
	area = GRAPHENE_RECT_INIT (0, 0, width, height);

	draw_surface (snapshot, page_texture, &area, inverted);

	/* Get the selection pixbuf iff we have something to draw */
	if (find_selection_for_page (page))
		selection_texture = pps_pixbuf_cache_get_selection_texture (priv->pixbuf_cache,
		                                                            priv->index,
		                                                            scale);

	if (selection_texture) {
		draw_surface (snapshot, selection_texture, &area, false);
	} else {
		region = pps_pixbuf_cache_get_selection_region (priv->pixbuf_cache,
		                                                priv->index,
		                                                scale);
		if (region) {
			GdkRGBA color;

			get_accent_color (&color, NULL);
			draw_selection_region (snapshot, region, &color);
		}
	}

	if (pps_search_context_get_active (priv->search_context))
		highlight_find_results (snapshot, page);

	if (pps_debug_get_debug_borders ())
		draw_debug_borders (snapshot, page, &area_rect);
}

static void
inverted_changed_cb (PpsDocumentModel *model,
                     GParamSpec *pspec,
                     PpsViewPage *page)
{
	if (pps_document_model_get_inverted_colors (model)) {
		gtk_widget_add_css_class (GTK_WIDGET (page), PPS_STYLE_CLASS_INVERTED);
	} else {
		gtk_widget_remove_css_class (GTK_WIDGET (page), PPS_STYLE_CLASS_INVERTED);
	}
}

static void
job_finished_cb (PpsPixbufCache *pixbuf_cache,
                 int finished_page,
                 PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	if (finished_page == priv->index)
		gtk_widget_queue_draw (GTK_WIDGET (page));
}

static void
search_results_changed_cb (PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);
	gboolean has_search_results = pps_search_context_has_results_on_page (priv->search_context, priv->index);

	/*
	 * If there are currently no search results shown (has_search_results) nor
	 * there are any to be shown in the next round (has_search_results), there
	 * is no reason to redraw. If either is true, we might have changes.
	 */
	if (has_search_results || priv->had_search_results)
		gtk_widget_queue_draw (GTK_WIDGET (page));

	priv->had_search_results = has_search_results;
}

static void
pps_view_page_init (PpsViewPage *page)
{
	gtk_widget_add_css_class (GTK_WIDGET (page), PPS_STYLE_CLASS_DOCUMENT_PAGE);
	gtk_widget_add_css_class (GTK_WIDGET (page), "card");
}

static void
pps_view_page_dispose (GObject *object)
{
	PpsViewPage *page = PPS_VIEW_PAGE (object);
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	g_clear_object (&priv->model);
	g_clear_object (&priv->pixbuf_cache);
	g_clear_object (&priv->page_cache);
	g_clear_object (&priv->search_context);

	G_OBJECT_CLASS (pps_view_page_parent_class)->dispose (object);
}

static void
pps_view_page_class_init (PpsViewPageClass *page_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (page_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (page_class);

	object_class->dispose = pps_view_page_dispose;
	widget_class->snapshot = pps_view_page_snapshot;
	widget_class->measure = pps_view_page_measure;
}

void
pps_view_page_setup (PpsViewPage *page,
                     PpsDocumentModel *model,
                     PpsSearchContext *search_context,
                     PpsPageCache *page_cache,
                     PpsPixbufCache *pixbuf_cache)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	g_return_if_fail (PPS_IS_VIEW (gtk_widget_get_parent (GTK_WIDGET (page))));

	if (priv->model != NULL)
		g_signal_handlers_disconnect_by_data (priv->model, page);
	if (priv->pixbuf_cache != NULL)
		g_signal_handlers_disconnect_by_data (priv->pixbuf_cache, page);
	if (priv->search_context != NULL)
		g_signal_handlers_disconnect_by_data (priv->search_context, page);

	g_set_object (&priv->model, model);
	g_set_object (&priv->search_context, search_context);
	g_set_object (&priv->page_cache, page_cache);
	g_set_object (&priv->pixbuf_cache, pixbuf_cache);
	pps_view_page_set_page (page, -1);

	g_signal_connect_data (priv->model, "notify::scale",
	                       G_CALLBACK (gtk_widget_queue_resize), page,
	                       NULL, G_CONNECT_SWAPPED);
	g_signal_connect (priv->model, "notify::inverted-colors",
	                  G_CALLBACK (inverted_changed_cb), page);
	g_signal_connect (priv->pixbuf_cache, "job-finished",
	                  G_CALLBACK (job_finished_cb), page);
	g_signal_connect_data (priv->search_context, "finished",
	                       G_CALLBACK (search_results_changed_cb), page,
	                       NULL, G_CONNECT_SWAPPED);
	g_signal_connect_data (priv->search_context, "result-activated",
	                       G_CALLBACK (search_results_changed_cb), page,
	                       NULL, G_CONNECT_SWAPPED);
	g_signal_connect_data (priv->search_context, "notify::active",
	                       G_CALLBACK (gtk_widget_queue_draw), page,
	                       NULL, G_CONNECT_SWAPPED);

	if (pps_document_model_get_inverted_colors (priv->model))
		gtk_widget_add_css_class (GTK_WIDGET (page), PPS_STYLE_CLASS_INVERTED);

	g_signal_connect_object (PPS_VIEW (gtk_widget_get_parent (GTK_WIDGET (page))),
	                         "selection-changed",
	                         G_CALLBACK (gtk_widget_queue_draw),
	                         page, G_CONNECT_SWAPPED);
}

PpsViewPage *
pps_view_page_new (void)
{
	return g_object_new (PPS_TYPE_VIEW_PAGE, NULL);
}

void
pps_view_page_set_page (PpsViewPage *page, gint index)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	g_return_if_fail (priv->model != NULL && pps_document_model_get_document (priv->model) != NULL);

	if (priv->index == index)
		return;

	if (0 <= index && index < pps_document_get_n_pages (pps_document_model_get_document (priv->model))) {
		priv->index = index;
	} else {
		priv->index = -1;
	}

	priv->had_search_results = FALSE;

	gtk_widget_queue_resize (GTK_WIDGET (page));
}

gint
pps_view_page_get_page (PpsViewPage *page)
{
	PpsViewPagePrivate *priv = GET_PRIVATE (page);

	return priv->index;
}
